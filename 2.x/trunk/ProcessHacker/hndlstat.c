/*
 * Process Hacker -
 *   handle statistics window
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

#include <phapp.h>

typedef struct _HANDLE_STATISTICS_ENTRY
{
    PPH_STRING Name;
    ULONG Count;
} HANDLE_STATISTICS_ENTRY, *PHANDLE_STATISTICS_ENTRY;

typedef struct _HANDLE_STATISTICS_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ProcessHandle;

    PSYSTEM_HANDLE_INFORMATION_EX Handles;
    HANDLE_STATISTICS_ENTRY Entries[MAX_OBJECT_TYPE_NUMBER];
} HANDLE_STATISTICS_CONTEXT, *PHANDLE_STATISTICS_CONTEXT;

INT_PTR CALLBACK PhpHandleStatisticsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowHandleStatisticsDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE_STATISTICS_CONTEXT context;
    BOOLEAN filterNeeded;
    ULONG i;

    context.ProcessId = ProcessId;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &context.ProcessHandle,
        PROCESS_DUP_HANDLE,
        ProcessId
        )))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process", status, 0);
        return;
    }

    status = PhEnumHandlesGeneric(
        context.ProcessId,
        context.ProcessHandle,
        &context.Handles,
        &filterNeeded
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(context.ProcessHandle);
        PhShowStatus(ParentWindowHandle, L"Unable to enumerate process handles", status, 0);
        return;
    }

    memset(&context.Entries, 0, sizeof(context.Entries));

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_HANDLESTATS),
        ParentWindowHandle,
        PhpHandleStatisticsDlgProc,
        (LPARAM)&context
        );

    for (i = 0; i < MAX_OBJECT_TYPE_NUMBER; i++)
    {
        if (context.Entries[i].Name)
            PhDereferenceObject(context.Entries[i].Name);
    }

    PhFree(context.Handles);
    NtClose(context.ProcessHandle);
}

static INT NTAPI PhpTypeCountCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PHANDLE_STATISTICS_ENTRY entry1 = Item1;
    PHANDLE_STATISTICS_ENTRY entry2 = Item2;

    return uintcmp(entry1->Count, entry2->Count);
}

INT_PTR CALLBACK PhpHandleStatisticsDlgProc(
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
            PHANDLE_STATISTICS_CONTEXT context = (PHANDLE_STATISTICS_CONTEXT)lParam;
            HANDLE processId;
            ULONG_PTR i;
            HWND lvHandle;

            processId = context->ProcessId;

            for (i = 0; i < context->Handles->NumberOfHandles; i++)
            {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo;
                PHANDLE_STATISTICS_ENTRY entry;
                PPH_STRING typeName;

                handleInfo = &context->Handles->Handles[i];

                if (handleInfo->UniqueProcessId != (ULONG_PTR)processId)
                    continue;
                if (handleInfo->ObjectTypeIndex >= MAX_OBJECT_TYPE_NUMBER)
                    continue;

                entry = &context->Entries[handleInfo->ObjectTypeIndex];

                if (!entry->Name)
                {
                    typeName = NULL;
                    PhGetHandleInformation(
                        context->ProcessHandle,
                        (HANDLE)handleInfo->HandleValue,
                        handleInfo->ObjectTypeIndex,
                        NULL,
                        &typeName,
                        NULL,
                        NULL
                        );
                    entry->Name = typeName;
                }

                entry->Count++;
            }

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Type");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Count");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetCompareFunction(lvHandle, 1, PhpTypeCountCompareFunction);

            for (i = 0; i < MAX_OBJECT_TYPE_NUMBER; i++)
            {
                PHANDLE_STATISTICS_ENTRY entry;
                PPH_STRING unknownType;
                PPH_STRING countString;
                INT lvItemIndex;

                entry = &context->Entries[i];

                if (entry->Count == 0)
                    continue;

                unknownType = NULL;

                if (!entry->Name)
                    unknownType = PhFormatString(L"(unknown: %u)", i);

                countString = PhFormatUInt64(entry->Count, TRUE);

                lvItemIndex = PhAddListViewItem(
                    lvHandle,
                    MAXINT,
                    entry->Name ? entry->Name->Buffer : unknownType->Buffer,
                    entry
                    );
                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, countString->Buffer);

                PhDereferenceObject(countString);

                if (unknownType)
                    PhDereferenceObject(unknownType);
            }

            ExtendedListView_SortItems(lvHandle);
        }
        break;
    case WM_DESTROY:
        {
            // Nothing
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}
