/*
 * Process Hacker -
 *   hidden processes detection
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

/*
 * There are two methods of hidden process detection implemented in this module.
 *
 * Brute Force. This attempts to open all possible PIDs within a certain range
 * in order to find processes which have been unlinked from the active process
 * list (EPROCESS.ActiveProcessLinks). This method is not effective when
 * either NtOpenProcess is hooked or PsLookupProcessByProcessId is hooked
 * (KProcessHacker cannot bypass this).
 *
 * CSR Handles. This enumerates handles in all running CSR processes, and works
 * even when a process has been unlinked from the active process list and
 * has been removed from the client ID table (PspCidTable). However, the method
 * does not detect native executables since CSR is not notified about them.
 * Some rootkits hook NtQuerySystemInformation in order to modify the returned
 * handle information; Process Hacker bypasses this by using KProcessHacker,
 * which calls ExEnumHandleTable directly. Note that both process and thread
 * handles are examined.
 */

#include <phapp.h>
#include <kphuser.h>
#include <settings.h>
#include <hidnproc.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpHiddenProcessesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

COLORREF NTAPI PhpHiddenProcessesColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in_opt PVOID Context
    );

BOOLEAN NTAPI PhpHiddenProcessesCallback(
    __in PPH_HIDDEN_PROCESS_ENTRY Process,
    __in_opt PVOID Context
    );

PPH_PROCESS_ITEM PhpCreateProcessItemForHiddenProcess(
    __in PPH_HIDDEN_PROCESS_ENTRY Entry
    );

HWND PhHiddenProcessesWindowHandle = NULL;
HWND PhHiddenProcessesListViewHandle = NULL;
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
    if (!KphIsConnected())
    {
        PhShowWarning(
            PhMainWndHandle,
            L"Hidden process detection cannot function properly without KProcessHacker. "
            L"Make sure Process Hacker is running with administrative privileges."
            );
    }

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
        SetForegroundWindow(PhHiddenProcessesWindowHandle);
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
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&WindowLayoutManager, lvHandle,
                NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM | PH_LAYOUT_FORCE_INVALIDATE);
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

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 330;
            MinimumSize.bottom = 140;
            MapDialogRect(hwndDlg, &MinimumSize);

            PhRegisterDialog(hwndDlg);

            PhLoadWindowPlacementFromSetting(L"HiddenProcessesWindowPosition", L"HiddenProcessesWindowSize", hwndDlg);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 320, L"Process");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"PID");

            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"HiddenProcessesListViewColumns", lvHandle);
            ExtendedListView_AddFallbackColumn(lvHandle, 0);
            ExtendedListView_AddFallbackColumn(lvHandle, 1);
            ExtendedListView_SetItemColorFunction(lvHandle, PhpHiddenProcessesColorFunction);

            ComboBox_AddString(GetDlgItem(hwndDlg, IDC_METHOD), L"Brute Force");
            ComboBox_AddString(GetDlgItem(hwndDlg, IDC_METHOD), L"CSR Handles");
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_METHOD), -1, L"CSR Handles");

            EnableWindow(GetDlgItem(hwndDlg, IDC_TERMINATE), FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(L"HiddenProcessesWindowPosition", L"HiddenProcessesWindowSize", hwndDlg);
            PhSaveListViewColumnsToSetting(L"HiddenProcessesListViewColumns", PhHiddenProcessesListViewHandle);
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
            case IDC_SCAN:
                {
                    NTSTATUS status;
                    PPH_STRING method;

                    method = PhGetWindowText(GetDlgItem(hwndDlg, IDC_METHOD));
                    PHA_DEREFERENCE(method);

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

                    ListView_DeleteAllItems(PhHiddenProcessesListViewHandle);

                    ProcessesList = PhCreateList(40);

                    ProcessesMethod =
                        PhEqualString2(method, L"Brute Force", TRUE) ?
                        BruteForceScanMethod :
                        CsrHandlesScanMethod;
                    NumberOfHiddenProcesses = 0;
                    NumberOfTerminatedProcesses = 0;

                    ExtendedListView_SetRedraw(PhHiddenProcessesListViewHandle, FALSE);
                    status = PhEnumHiddenProcesses(
                        ProcessesMethod,
                        PhpHiddenProcessesCallback,
                        NULL
                        );
                    ExtendedListView_SortItems(PhHiddenProcessesListViewHandle);
                    ExtendedListView_SetRedraw(PhHiddenProcessesListViewHandle, TRUE);

                    if (NT_SUCCESS(status))
                    {
                        SetDlgItemText(hwndDlg, IDC_DESCRIPTION,
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
                                LARGE_INTEGER interval;

                                // Sleep for a bit before continuing. It seems to help avoid
                                // BSODs.
                                interval.QuadPart = -250 * PH_TIMEOUT_MS;
                                NtDelayExecution(FALSE, &interval);
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

                        fileName = PhGetFileDialogFileName(fileDialog);
                        PhaDereferenceObject(fileName);

                        if (NT_SUCCESS(status = PhCreateFileStream(
                            &fileStream,
                            fileName->Buffer,
                            FILE_GENERIC_WRITE,
                            FILE_SHARE_READ,
                            FILE_OVERWRITE_IF,
                            0
                            )))
                        {
                            PhWritePhTextHeader(fileStream);
                            PhWriteStringAsAnsiFileStream2(fileStream, L"Method: ");
                            PhWriteStringAsAnsiFileStream2(fileStream,
                                ProcessesMethod == BruteForceScanMethod ? L"Brute Force\r\n" : L"CSR Handles\r\n");
                            PhWriteStringFormatFileStream(
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
                                        PhWriteStringAsAnsiFileStream2(fileStream, L"[HIDDEN] ");
                                    else if (entry->Type == TerminatedProcess)
                                        PhWriteStringAsAnsiFileStream2(fileStream, L"[Terminated] ");
                                    else if (entry->Type != NormalProcess)
                                        continue;

                                    PhWriteStringFormatFileStream(
                                        fileStream,
                                        L"%s (%u)\r\n",
                                        entry->FileName->Buffer,
                                        (ULONG)entry->ProcessId
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
                                ProcessHacker_ShowProcessProperties(PhMainWndHandle, processItem);
                                PhDereferenceObject(processItem);
                            }
                            else
                            {
                                PhShowError(hwndDlg, L"Unable to create a process structure for the selected process.");
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
    __in INT Index,
    __in PVOID Param,
    __in_opt PVOID Context
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

    return PhSysWindowColor;
}

static BOOLEAN NTAPI PhpHiddenProcessesCallback(
    __in PPH_HIDDEN_PROCESS_ENTRY Process,
    __in_opt PVOID Context
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
    PhPrintUInt32(pidString, (ULONG)entry->ProcessId);
    PhSetListViewSubItem(PhHiddenProcessesListViewHandle, lvItemIndex, 1, pidString);

    if (entry->Type == HiddenProcess)
        NumberOfHiddenProcesses++;
    else if (entry->Type == TerminatedProcess)
        NumberOfTerminatedProcesses++;

    return TRUE;
}

static PPH_PROCESS_ITEM PhpCreateProcessItemForHiddenProcess(
    __in PPH_HIDDEN_PROCESS_ENTRY Entry
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem;
    PPH_PROCESS_ITEM idleProcessItem;
    HANDLE processHandle;
    PROCESS_BASIC_INFORMATION basicInfo;
    KERNEL_USER_TIMES times;
    PROCESS_PRIORITY_CLASS priorityClass;
    ULONG handleCount;
    HANDLE processHandle2;

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
            ProcessQueryAccess,
            Entry->ProcessId
            );
    }
    else
    {
        status = PhOpenProcessByCsrHandles(
            &processHandle,
            ProcessQueryAccess,
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

        PhPrintUInt32(processItem->ParentProcessIdString, (ULONG)processItem->ParentProcessId);
        PhPrintUInt32(processItem->SessionIdString, processItem->SessionId);

        if (NT_SUCCESS(PhGetProcessTimes(processHandle, &times)))
        {
            processItem->CreateTime = times.CreateTime;
            processItem->KernelTime = times.KernelTime;
            processItem->UserTime = times.UserTime;
        }

        // TODO: Token information?

        if (NT_SUCCESS(NtQueryInformationProcess(
            processHandle,
            ProcessPriorityClass,
            &priorityClass,
            sizeof(PROCESS_PRIORITY_CLASS),
            NULL
            )))
        {
            processItem->PriorityClass = priorityClass.PriorityClass;
        }

        if (NT_SUCCESS(NtQueryInformationProcess(
            processHandle,
            ProcessHandleCount,
            &handleCount,
            sizeof(ULONG),
            NULL
            )))
        {
            processItem->NumberOfHandles = handleCount;
        }
    }

    // Stage 1
    // Some copy and paste magic here...

    if (processItem->FileName)
    {
        // Small icon, large icon.
        ExtractIconEx(
            processItem->FileName->Buffer,
            0,
            &processItem->LargeIcon,
            &processItem->SmallIcon,
            1
            );

        // Version info.
        PhInitializeImageVersionInfo(&processItem->VersionInfo, processItem->FileName->Buffer);
    }

    // Use the default EXE icon if we didn't get the file's icon.
    {
        if (!processItem->SmallIcon || !processItem->LargeIcon)
        {
            if (processItem->SmallIcon)
            {
                DestroyIcon(processItem->SmallIcon);
                processItem->SmallIcon = NULL;
            }
            else if (processItem->LargeIcon)
            {
                DestroyIcon(processItem->LargeIcon);
                processItem->LargeIcon = NULL;
            }

            PhGetStockApplicationIcon(&processItem->SmallIcon, &processItem->LargeIcon);
            processItem->SmallIcon = DuplicateIcon(NULL, processItem->SmallIcon);
            processItem->LargeIcon = DuplicateIcon(NULL, processItem->LargeIcon);
        }
    }

    // POSIX, command line

    status = PhOpenProcess(
        &processHandle2,
        ProcessQueryAccess | PROCESS_VM_READ,
        Entry->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        BOOLEAN isPosix = FALSE;
        PPH_STRING commandLine;
        ULONG i;

        status = PhGetProcessIsPosix(processHandle2, &isPosix);
        processItem->IsPosix = isPosix;

        if (!NT_SUCCESS(status) || !isPosix)
        {
            status = PhGetProcessCommandLine(processHandle2, &commandLine);

            if (NT_SUCCESS(status))
            {
                // Some command lines (e.g. from taskeng.exe) have nulls in them.
                // Since Windows can't display them, we'll replace them with
                // spaces.
                for (i = 0; i < (ULONG)commandLine->Length / 2; i++)
                {
                    if (commandLine->Buffer[i] == 0)
                        commandLine->Buffer[i] = ' ';
                }
            }
        }
        else
        {
            // Get the POSIX command line.
            status = PhGetProcessPosixCommandLine(processHandle2, &commandLine);
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
    __in PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    __in_opt PVOID Context
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
            ProcessQueryAccess,
            (HANDLE)pid
            );

        if (NT_SUCCESS(status2))
        {
            entry.ProcessId = (HANDLE)pid;

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
                else if (PhFindItemList(pids, (HANDLE)pid) != -1)
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
        if (status2 == STATUS_ACCESS_DENIED && WindowsVersion >= WINDOWS_VISTA)
        {
            if (NT_SUCCESS(status2 = PhGetProcessImageFileNameByProcessId((HANDLE)pid, &fileName)))
            {
                entry.ProcessId = (HANDLE)pid;
                entry.FileName = PhGetFileName(fileName);
                PhDereferenceObject(fileName);

                if (PhFindItemList(pids, (HANDLE)pid) != -1)
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
            entry.ProcessId = (HANDLE)pid;
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
    __in PPH_CSR_HANDLE_INFO Handle,
    __in_opt PVOID Context
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
        ProcessQueryAccess,
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
            else if (PhFindItemList(context->Pids, Handle->ProcessId) != -1)
                entry.Type = NormalProcess;
            else
                entry.Type = HiddenProcess;

            if (!context->Callback(&entry, context->Context))
                cont = FALSE;

            PhDereferenceObject(entry.FileName);
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        entry.FileName = NULL;
        entry.Type = UnknownProcess;

        if (!context->Callback(&entry, context->Context))
            cont = FALSE;
    }

    return cont;
}

NTSTATUS PhpEnumHiddenProcessesCsrHandles(
    __in PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    __in_opt PVOID Context
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

NTSTATUS PhEnumHiddenProcesses(
    __in PH_HIDDEN_PROCESS_METHOD Method,
    __in PPH_ENUM_HIDDEN_PROCESSES_CALLBACK Callback,
    __in_opt PVOID Context
    )
{
    if (Method == BruteForceScanMethod)
    {
        return PhpEnumHiddenProcessesBruteForce(
            Callback,
            Context
            );
    }
    else
    {
        return PhpEnumHiddenProcessesCsrHandles(
            Callback,
            Context
            );
    }
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
    __in PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK Callback,
    __in_opt PVOID Context
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
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PPH_CSR_HANDLE_INFO Handle
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
    __in PPH_CSR_HANDLE_INFO Handle,
    __in_opt PVOID Context
    )
{
    POPEN_PROCESS_BY_CSR_CONTEXT context = Context;

    if (Handle->ProcessId == context->ProcessId)
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
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessId
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
