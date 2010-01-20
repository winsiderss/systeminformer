/*
 * Process Hacker - 
 *   process termination tool
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
    __in PPH_PROCESS_ITEM Process
    )
{
    PhReferenceObject(Process);
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_TERMINATOR),
        ParentWindowHandle,
        PhpProcessTerminatorDlgProc,
        (LPARAM)Process
        );
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

        CloseHandle(processHandle);
    }

    return status;
}

TEST_ITEM PhTerminatorTests[] =
{
    { L"TP1", L"Terminates the process using NtTerminateProcess", TerminatorTP1 }
};

static VOID PhpRunTerminatorTest(
    __in HWND WindowHandle,
    __in INT Index
    )
{
    NTSTATUS status;
    PTEST_PROC param;
    PPH_PROCESS_ITEM processItem;
    HWND lvHandle;
    PVOID processes;

    processItem = (PPH_PROCESS_ITEM)GetProp(WindowHandle, L"ProcessItem");
    lvHandle = GetDlgItem(WindowHandle, IDC_TERMINATOR_LIST);

    if (!PhGetListViewItemParam(
        lvHandle,
        Index,
        (PPVOID)&param
        ))
        return;

    status = param(processItem->ProcessId);
    Sleep(1000);

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    // Check if the process exists.
    if (!PhFindProcessInformation(processes, processItem->ProcessId))
    {
        PhSetListViewItemImageIndex(lvHandle, Index, 1); // tick
        SetDlgItemText(WindowHandle, IDC_TERMINATOR_TEXT, L"The process was terminated.");
    }
    else
    {
        PhSetListViewItemImageIndex(lvHandle, Index, 0); // cross
    }

    PhFree(processes);
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
            HWND lvHandle;
            HIMAGELIST imageList;
            ULONG i;

            SetProp(hwndDlg, L"ProcessItem", (HANDLE)processItem);

            lvHandle = GetDlgItem(hwndDlg, IDC_TERMINATOR_LIST);
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"ID");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Description");
            ListView_SetExtendedListViewStyleEx(
                lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
                -1
                );
            PhSetControlTheme(lvHandle, L"explorer");

            imageList = ImageList_Create(16, 16, ILC_COLOR32, 2, 2);
            PhAddImageListBitmap(imageList, PhInstanceHandle, MAKEINTRESOURCE(IDB_CROSS));
            PhAddImageListBitmap(imageList, PhInstanceHandle, MAKEINTRESOURCE(IDB_TICK));
            ListView_SetImageList(lvHandle, imageList, LVSIL_SMALL);

            for (i = 0; i < sizeof(PhTerminatorTests) / sizeof(TEST_ITEM); i++)
            {
                INT itemIndex;

                itemIndex = PhAddListViewItem(
                    lvHandle,
                    MAXINT,
                    PhTerminatorTests[i].Id,
                    PhTerminatorTests[i].TestProc
                    );
                PhSetListViewSubItem(lvHandle, itemIndex, 1, PhTerminatorTests[i].Description);
            }

            SetDlgItemText(
                hwndDlg,
                IDC_TERMINATOR_TEXT,
                L"Double-click a termination method or click Run All."
                );
        }
        break;
    case WM_DESTROY:
        {
            PhDereferenceObject((PPH_PROCESS_ITEM)GetProp(hwndDlg, L"ProcessItem"));
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
                            PhpRunTerminatorTest(
                                hwndDlg,
                                i
                                );
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
