/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2019-2023
 *
 */

/*
 * There are two methods of zombie process detection implemented in this module.
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
 * handle information; System Informer bypasses this by using KSystemInformer,
 * which calls ExEnumHandleTable directly. Note that both process and thread
 * handles are examined.
 */

#include <phapp.h>
#include <hidnproc.h>
#include <hndlinfo.h>
#include <kphuser.h>
#include <mainwnd.h>
#include <procprv.h>
#include <settings.h>
#include <phsettings.h>
#include <emenu.h>

INT_PTR CALLBACK PhpZombieProcessesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

COLORREF NTAPI PhpZombieProcessesColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );

BOOLEAN NTAPI PhpZombieProcessesCallback(
    _In_ PPH_ZOMBIE_PROCESS_ENTRY Process,
    _In_opt_ PVOID Context
    );

VOID PhZombieProcessesUpdateListView(
    _In_ PPH_LIST UpdateList
    );

NTSTATUS PhpCreateProcessItemForZombieProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_ZOMBIE_PROCESS_ENTRY Entry,
    _Out_ PPH_PROCESS_ITEM* ProcessItem
    );

static HWND PhZombieProcessesWindowHandle = NULL;
static HWND PhZombieProcessesListViewHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;

static PH_ZOMBIE_PROCESS_METHOD ProcessesMethod;
static PPH_LIST ProcessesList = NULL;
static ULONG NumberOfZombieProcesses;
static ULONG NumberOfTerminatedProcesses;

VOID PhShowZombieProcessesDialog(
    VOID
    )
{
    if (!PhZombieProcessesWindowHandle)
    {
        PhZombieProcessesWindowHandle = PhCreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_ZOMBIEPROCESSES),
            NULL,
            PhpZombieProcessesDlgProc,
            NULL
            );
    }

    if (!IsWindowVisible(PhZombieProcessesWindowHandle))
        ShowWindow(PhZombieProcessesWindowHandle, SW_SHOW);
    else
        SetForegroundWindow(PhZombieProcessesWindowHandle);
}

VOID PhZombieProcessesCleanupList(
    _In_ PPH_LIST UpdateList
    )
{
    if (UpdateList)
    {
        for (ULONG i = 0; i < UpdateList->Count; i++)
        {
            PPH_ZOMBIE_PROCESS_ENTRY entry = UpdateList->Items[i];

            PhClearReference(&entry->FileName);
            PhFree(entry);
        }

        PhDereferenceObject(UpdateList);
    }
    {
        PPH_STRING string = PhFormatString(L"%u zombie process(es), %u terminated process(es).",
            NumberOfZombieProcesses, NumberOfTerminatedProcesses);
        PhSetDialogItemText(PhZombieProcessesWindowHandle, IDC_DESCRIPTION, string->Buffer);
        InvalidateRect(GetDlgItem(PhZombieProcessesWindowHandle, IDC_DESCRIPTION), NULL, TRUE);
        PhDereferenceObject(string);
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
 NTSTATUS PhZombieProcessesThread(
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    PPH_LIST processList;

    ExtendedListView_SetRedraw(PhZombieProcessesListViewHandle, FALSE);
    ListView_DeleteAllItems(PhZombieProcessesListViewHandle);
    ExtendedListView_SetRedraw(PhZombieProcessesListViewHandle, TRUE);

    processList = PhCreateList(100);

    status = PhEnumZombieProcesses(
        ProcessesMethod,
        PhpZombieProcessesCallback,
        processList
        );

    if (PhZombieProcessesWindowHandle)
        PostMessage(PhZombieProcessesWindowHandle, WM_PH_UPDATE_DIALOG, status, (LPARAM)processList);
    else
        PhZombieProcessesCleanupList(processList);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpZombieProcessesDlgProc(
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

            PhZombieProcessesListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_PROCESSES);
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
            PhLoadListViewColumnsFromSetting(L"ZombieProcessesListViewColumns", lvHandle);
            ExtendedListView_AddFallbackColumn(lvHandle, 0);
            ExtendedListView_AddFallbackColumn(lvHandle, 1);
            ExtendedListView_SetItemColorFunction(lvHandle, PhpZombieProcessesColorFunction);

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

            if (PhValidWindowPlacementFromSetting(L"ZombieProcessesWindowPosition"))
                PhLoadWindowPlacementFromSetting(L"ZombieProcessesWindowPosition", L"ZombieProcessesWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            EnableWindow(GetDlgItem(hwndDlg, IDC_TERMINATE), FALSE);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(L"ZombieProcessesWindowPosition", L"ZombieProcessesWindowSize", hwndDlg);
            PhSaveListViewColumnsToSetting(L"ZombieProcessesListViewColumns", PhZombieProcessesListViewHandle);

            PhZombieProcessesCleanupList(ProcessesList);

            PhZombieProcessesWindowHandle = NULL;
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
                    PPH_STRING method;

                    method = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_METHOD)));

                    PhZombieProcessesCleanupList(NULL);

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

                    NumberOfZombieProcesses = 0;
                    NumberOfTerminatedProcesses = 0;

                    EnableWindow(GetDlgItem(hwndDlg, IDC_SCAN), FALSE);

                    PhCreateThread2(PhZombieProcessesThread, NULL);
                }
                break;
            case IDC_TERMINATE:
                {
                    PPH_ZOMBIE_PROCESS_ENTRY *entries;
                    ULONG numberOfEntries;
                    ULONG i;

                    PhGetSelectedListViewItemParams(PhZombieProcessesListViewHandle, &entries, &numberOfEntries);

                    if (numberOfEntries != 0)
                    {
                        if (!PhGetIntegerSetting(L"EnableWarnings") ||
                            PhShowConfirmMessage(
                            hwndDlg,
                            L"terminate",
                            L"the selected process(es)",
                            L"Terminating a Zombie process may cause the system to become unstable "
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
                                if (ProcessesMethod == BruteForceScanMethod || ProcessesMethod == ProcessHandleScanMethod)
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
                    PhSetFileDialogFileName(fileDialog, L"Zombie Processes.txt");

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
                            PhWriteStringAsUtf8FileStream(fileStream, (PPH_STRINGREF)&PhUnicodeByteOrderMark);
                            PhWritePhTextHeader(fileStream);
                            PhWriteStringAsUtf8FileStream2(fileStream, L"Method: ");
                            PhWriteStringAsUtf8FileStream2(fileStream,
                                ProcessesMethod == BruteForceScanMethod ? L"Brute Force\r\n" : L"CSR Handles\r\n");
                            PhWriteStringFormatAsUtf8FileStream(
                                fileStream,
                                L"Zombie: %u\r\nTerminated: %u\r\n\r\n",
                                NumberOfZombieProcesses,
                                NumberOfTerminatedProcesses
                                );

                            if (ProcessesList)
                            {
                                ULONG i;

                                for (i = 0; i < ProcessesList->Count; i++)
                                {
                                    PPH_ZOMBIE_PROCESS_ENTRY entry = ProcessesList->Items[i];

                                    if (entry->Type == ZombieProcess)
                                        PhWriteStringAsUtf8FileStream2(fileStream, L"[Zombie] ");
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

            PhHandleListViewNotifyBehaviors(lParam, PhZombieProcessesListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == PhZombieProcessesListViewHandle)
                    {
                        EnableWindow(
                            GetDlgItem(hwndDlg, IDC_TERMINATE),
                            ListView_GetSelectedCount(PhZombieProcessesListViewHandle) > 0
                            );
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == PhZombieProcessesListViewHandle)
                    {
                        PPH_ZOMBIE_PROCESS_ENTRY entry;

                        entry = PhGetSelectedListViewItemParam(PhZombieProcessesListViewHandle);

                        if (entry)
                        {
                            NTSTATUS status;
                            PPH_PROCESS_ITEM processItem;

                            status = PhpCreateProcessItemForZombieProcess(hwndDlg, entry, &processItem);

                            if (NT_SUCCESS(status))
                            {
                                SystemInformer_ShowProcessProperties(processItem);
                                PhDereferenceObject(processItem);
                            }
                            else
                            {
                                PhShowStatus(hwndDlg, L"Unable to create a process structure for the selected process.", status, 0);
                            }
                        }
                    }
                }
                break;
            }

            REFLECT_MESSAGE_DLG(hwndDlg, PhZombieProcessesListViewHandle, uMsg, wParam, lParam);
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
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == PhZombieProcessesListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(PhZombieProcessesListViewHandle, &point);

                PhGetSelectedListViewItemParams(PhZombieProcessesListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, PhZombieProcessesListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                PhCopyListView(PhZombieProcessesListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            NTSTATUS status = (NTSTATUS)wParam;
            PPH_LIST list = (PPH_LIST)lParam;

            ExtendedListView_SetRedraw(PhZombieProcessesListViewHandle, FALSE);
            ListView_DeleteAllItems(PhZombieProcessesListViewHandle);
            PhZombieProcessesUpdateListView(list);
            ExtendedListView_SortItems(PhZombieProcessesListViewHandle);
            ExtendedListView_SetRedraw(PhZombieProcessesListViewHandle, TRUE);

            if (NT_SUCCESS(status))
            {
                PhSetDialogItemText(hwndDlg, IDC_DESCRIPTION, PhaFormatString(
                    L"%u zombie process(es), %u terminated process(es).",
                    NumberOfZombieProcesses,
                    NumberOfTerminatedProcesses
                    )->Buffer);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_DESCRIPTION), NULL, TRUE);
            }
            else
            {
                PhShowStatus(hwndDlg, L"Unable to perform the scan", status, 0);
            }

            EnableWindow(GetDlgItem(hwndDlg, IDC_SCAN), TRUE);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        {
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DESCRIPTION))
            {
                if (NumberOfZombieProcesses != 0)
                {
                    SetTextColor((HDC)wParam, RGB(0xff, 0x00, 0x00));
                }

                SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));

                return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
            }

            return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
        }
    }

    return FALSE;
}

COLORREF NTAPI PhpZombieProcessesColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PPH_ZOMBIE_PROCESS_ENTRY entry = Param;

    switch (entry->Type)
    {
    case UnknownProcess:
    case ZombieProcess:
        return RGB(229, 186, 208);
    case TerminatedProcess:
        return RGB(0x77, 0x77, 0x77);
    }

    return PhEnableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_WINDOW);
}

BOOLEAN NTAPI PhpZombieProcessesCallback(
    _In_ PPH_ZOMBIE_PROCESS_ENTRY Process,
    _In_ PVOID Context
    )
{
    PPH_ZOMBIE_PROCESS_ENTRY entry;

    for (ULONG i = 0; i < ((PPH_LIST)Context)->Count; i++)
    {
        PPH_ZOMBIE_PROCESS_ENTRY item = ((PPH_LIST)Context)->Items[i];

        if (item->ProcessId == Process->ProcessId)
        {
            return TRUE; // duplicate
        }
    }

    entry = PhAllocateCopy(Process, sizeof(PH_ZOMBIE_PROCESS_ENTRY));

    if (entry->FileName)
        PhReferenceObject(entry->FileName);

    PhAddItemList(Context, entry);

    if (entry->Type == ZombieProcess)
        InterlockedIncrement(&NumberOfZombieProcesses);
    else if (entry->Type == TerminatedProcess)
        InterlockedIncrement(&NumberOfTerminatedProcesses);

    return TRUE;
}

VOID PhZombieProcessesUpdateListView(
    _In_ PPH_LIST UpdateList
    )
{
    for (ULONG i = 0; i < UpdateList->Count; i++)
    {
        PPH_ZOMBIE_PROCESS_ENTRY entry = UpdateList->Items[i];
        INT lvItemIndex;
        WCHAR pidString[PH_INT32_STR_LEN_1];

        if (entry->FileName)
        {
            PhMoveReference(&entry->FileName, PhGetFileName(entry->FileName));
        }

        lvItemIndex = PhAddListViewItem(
            PhZombieProcessesListViewHandle,
            MAXINT,
            PhGetStringOrDefault(entry->FileName, L"(unknown)"),
            entry
            );
        PhPrintUInt32(pidString, HandleToUlong(entry->ProcessId));
        PhSetListViewSubItem(PhZombieProcessesListViewHandle, lvItemIndex, 1, pidString);

        PhAddItemList(ProcessesList, entry);
    }
}

NTSTATUS PhpCreateProcessItemForZombieProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_ZOMBIE_PROCESS_ENTRY Entry,
    _Out_ PPH_PROCESS_ITEM* ProcessItem
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem;
    HANDLE processHandle;

    if (Entry->Type == NormalProcess)
    {
        if (processItem = PhReferenceProcessItem(Entry->ProcessId))
        {
            *ProcessItem = processItem;
            return STATUS_SUCCESS;
        }
    }

    if (ProcessesMethod != CsrHandlesScanMethod)
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
        if (processItem = PhCreateProcessItemFromHandle(
            Entry->ProcessId,
            processHandle,
            Entry->Type == TerminatedProcess
            ))
        {
            *ProcessItem = processItem;
            return STATUS_SUCCESS;
        }

        NtClose(processHandle);
    }

    return status;
}

NTSTATUS PhpEnumZombieProcessesBruteForce(
    _In_ PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback,
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
        PH_ZOMBIE_PROCESS_ENTRY entry;
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
                entry.FileName = fileName;

                if (times.ExitTime.QuadPart != 0)
                    entry.Type = TerminatedProcess;
                else if (PhFindItemList(pids, UlongToHandle(pid)) != ULONG_MAX)
                    entry.Type = NormalProcess;
                else
                    entry.Type = ZombieProcess;

                if (!Callback(&entry, Context))
                    stop = TRUE;

                PhDereferenceObject(fileName);
            }

            NtClose(processHandle);
        }

        // Use an alternative method if we don't have sufficient access.
        if (status2 == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status2 = PhGetProcessImageFileNameByProcessId(UlongToHandle(pid), &fileName)))
            {
                entry.ProcessId = UlongToHandle(pid);
                entry.FileName = fileName;

                if (PhFindItemList(pids, UlongToHandle(pid)) != ULONG_MAX)
                    entry.Type = NormalProcess;
                else
                    entry.Type = ZombieProcess;

                if (!Callback(&entry, Context))
                    stop = TRUE;

                PhDereferenceObject(fileName);
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
    PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback;
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
    PH_ZOMBIE_PROCESS_ENTRY entry;

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
            entry.FileName = fileName;

            if (times.ExitTime.QuadPart != 0)
                entry.Type = TerminatedProcess;
            else if (context && PhFindItemList(context->Pids, Handle->ProcessId) != ULONG_MAX)
                entry.Type = NormalProcess;
            else
                entry.Type = ZombieProcess;

            if (context && !context->Callback(&entry, context->Context))
                cont = FALSE;

            PhDereferenceObject(fileName);
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

NTSTATUS PhpEnumZombieProcessesCsrHandles(
    _In_ PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback,
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

typedef struct _PH_ENUM_NEXT_PROCESS_CONTEXT
{
    PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback;
    PVOID Context;
} PH_ENUM_NEXT_PROCESS_CONTEXT, *PPH_ENUM_NEXT_PROCESS_CONTEXT;

NTSTATUS NTAPI PhpEnumNextProcessHandles(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID Context
    )
{
    PPH_ENUM_NEXT_PROCESS_CONTEXT context = Context;
    PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;
    NTSTATUS status;
    PVOID processes = NULL;

    status = PhGetProcessExtendedBasicInformation(ProcessHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        status = PhEnumProcesses(&processes);

        if (NT_SUCCESS(status))
        {
            if (!PhFindProcessInformation(processes, basicInfo.BasicInfo.UniqueProcessId))
            {
                PH_ZOMBIE_PROCESS_ENTRY entry;
                PPH_STRING fileName;

                entry.ProcessId = basicInfo.BasicInfo.UniqueProcessId;

                if (NT_SUCCESS(PhGetProcessImageFileName(ProcessHandle, &fileName)))
                {
                    entry.FileName = fileName;
                    entry.Type = ZombieProcess;

                    //if (basicInfo.IsProcessDeleting)
                    //    entry.Type = TerminatedProcess;

                    if (!context->Callback(&entry, context->Context))
                        goto CleanupExit;

                    PhDereferenceObject(fileName);
                }
                else
                {
                    entry.FileName = NULL;
                    entry.Type = UnknownProcess;

                    if (!context->Callback(&entry, context->Context))
                        goto CleanupExit;
                }
            }
        }
    }

CleanupExit:
    if (processes)
    {
        PhFree(processes);
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhpEnumZombieProcessHandles(
    _In_ PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PH_ENUM_NEXT_PROCESS_CONTEXT context;

    context.Callback = Callback;
    context.Context = Context;

    status = PhEnumNextProcess(
        NULL,
        PROCESS_QUERY_LIMITED_INFORMATION,
        PhpEnumNextProcessHandles,
        &context
        );

    return status;
}

NTSTATUS PhpEnumZombieSubKeyHandles(
    _In_ PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PKEY_OPEN_SUBKEYS_INFORMATION buffer;
    UNICODE_STRING subkeyName;
    OBJECT_ATTRIBUTES objectAttributes;

    bufferSize = 0x200;
    buffer = PhAllocate(bufferSize);

    RtlInitUnicodeString(&subkeyName, L"\\REGISTRY");
    InitializeObjectAttributes(
        &objectAttributes,
        &subkeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    while (TRUE)
    {
        status = NtQueryOpenSubKeysEx(
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
                        PH_ZOMBIE_PROCESS_ENTRY process;
                        PPH_STRING fileName;

                        process.ProcessId = entry.ProcessId;

                        if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                        {
                            PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

                            process.FileName = fileName;
                            process.Type = ZombieProcess;

                            if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
                            {
                                if (basicInfo.IsProcessDeleting)
                                    process.Type = TerminatedProcess;
                            }

                            if (!Callback(&process, Context))
                                break;

                            PhDereferenceObject(fileName);
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
                PH_ZOMBIE_PROCESS_ENTRY process;
                PPH_STRING fileName;

                process.ProcessId = entry.ProcessId;

                if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(process.ProcessId, &fileName)))
                {
                    process.FileName = fileName;
                    process.Type = ZombieProcess;

                    if (!Callback(&process, Context))
                        break;

                    PhDereferenceObject(fileName);
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
    (((PETW_TRACE_GUID_INFO)(TraceGuid))->InstanceCount ? \
    ((PETW_TRACE_PROVIDER_INSTANCE_INFO)PTR_ADD_OFFSET(TraceGuid, \
    sizeof(ETW_TRACE_GUID_INFO))) : NULL)
#define PH_NEXT_ETW_GUID(TraceGuid) \
    (((PETW_TRACE_PROVIDER_INSTANCE_INFO)(TraceGuid))->NextOffset ? \
    (PETW_TRACE_PROVIDER_INSTANCE_INFO)PTR_ADD_OFFSET((TraceGuid), \
    ((PETW_TRACE_PROVIDER_INSTANCE_INFO)(TraceGuid))->NextOffset) : NULL)

NTSTATUS PhpEnumEtwGuidHandles(
    _In_ PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PGUID traceGuidList = NULL;
    ULONG traceGuidListLength = 0;

    status = PhTraceControlVariableSize(
        EtwEnumTraceGuidList,
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
            PETW_TRACE_GUID_INFO traceGuidInfo;

            status = PhTraceControlVariableSize(
                EtwGetTraceGuidInfo,
                &providerGuid,
                sizeof(GUID),
                &traceGuidInfo,
                NULL
                );

            if (NT_SUCCESS(status))
            {
                PETW_TRACE_PROVIDER_INSTANCE_INFO instance;
                HANDLE processHandle;
                //PVOID processes;

                for (instance = PH_FIRST_ETW_GUID(traceGuidInfo);
                    instance;
                    instance = PH_NEXT_ETW_GUID(instance))
                {
                    if (instance->Pid == 0)
                        continue;

                    if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, UlongToHandle(instance->Pid))))
                    {
                        PPH_PROCESS_ITEM processItem;

                        processItem = PhReferenceProcessItem(UlongToHandle(instance->Pid));
         
                        //if (NT_SUCCESS(PhEnumProcesses(&processes)))
                        {
                            //if (!PhFindProcessInformation(processes, UlongToHandle(instance->Pid)))
                            if (!processItem)
                            {
                                PH_ZOMBIE_PROCESS_ENTRY process;
                                PPH_STRING fileName;

                                process.ProcessId = UlongToHandle(instance->Pid);

                                if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                                {
                                    PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

                                    process.FileName = fileName;
                                    process.Type = ZombieProcess;

                                    if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
                                    {
                                        if (basicInfo.IsProcessDeleting)
                                            process.Type = TerminatedProcess;
                                    }

                                    if (!Callback(&process, Context))
                                        break;

                                    PhDereferenceObject(fileName);
                                }
                                else
                                {
                                    process.FileName = NULL;
                                    process.Type = UnknownProcess;

                                    if (!Callback(&process, Context))
                                        break;
                                }
                            }

                            //PhFree(processes);
                        }

                        PhClearReference(&processItem);

                        NtClose(processHandle);
                    }
                    else
                    {
                        PH_ZOMBIE_PROCESS_ENTRY process;
                        PPH_STRING fileName;

                        process.ProcessId = UlongToHandle(instance->Pid);

                        if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(process.ProcessId, &fileName)))
                        {
                            process.FileName = fileName;
                            process.Type = ZombieProcess;

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
    _In_ PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    static CONST PH_STRINGREF ntdllPath = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll");
    static CONST PH_STRINGREF ntfsPath = PH_STRINGREF_INIT(L"\\ntfs");
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
                            PH_ZOMBIE_PROCESS_ENTRY process;
                            PPH_STRING fileName;

                            process.ProcessId = processId;

                            if (NT_SUCCESS(PhGetProcessImageFileName(processHandle, &fileName)))
                            {
                                PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

                                process.FileName = fileName;
                                process.Type = ZombieProcess;

                                if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInfo)))
                                {
                                    //if (basicInfo.IsProcessDeleting)
                                    //    process.Type = TerminatedProcess;
                                }

                                if (!Callback(&process, Context))
                                    break;

                                PhDereferenceObject(fileName);
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
                    PH_ZOMBIE_PROCESS_ENTRY process;
                    PPH_STRING fileName;

                    process.ProcessId = processId;

                    if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(process.ProcessId, &fileName)))
                    {
                        process.FileName = fileName;
                        process.Type = ZombieProcess;

                        if (!Callback(&process, Context))
                            break;

                        PhDereferenceObject(fileName);
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

NTSTATUS PhEnumZombieProcesses(
    _In_ PH_ZOMBIE_PROCESS_METHOD Method,
    _In_ PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    switch (Method)
    {
    case BruteForceScanMethod:
        return PhpEnumZombieProcessesBruteForce(Callback, Context);
    case CsrHandlesScanMethod:
        return PhpEnumZombieProcessesCsrHandles(Callback, Context);
    case ProcessHandleScanMethod:
        return PhpEnumZombieProcessHandles(Callback, Context);
    case RegistryScanMethod:
        return PhpEnumZombieSubKeyHandles(Callback, Context);
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

        if (NT_SUCCESS(KsiEnumerateProcessHandles(csrProcessHandles[i], &handles)))
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
                if (PhFindItemList(pids, handle.ProcessId) != ULONG_MAX)
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
