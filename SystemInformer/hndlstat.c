/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2022-2023
 *
 */

#include <phapp.h>
#include <hndlinfo.h>
#include <settings.h>
#include <phsettings.h>

typedef struct _HANDLE_STATISTICS_ENTRY
{
    PPH_STRING Name;
    ULONG Count;
} HANDLE_STATISTICS_ENTRY, *PHANDLE_STATISTICS_ENTRY;

typedef struct _HANDLE_STATISTICS_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ProcessHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PSYSTEM_HANDLE_INFORMATION_EX Handles;
    HANDLE_STATISTICS_ENTRY Entries[MAX_OBJECT_TYPE_NUMBER];
} HANDLE_STATISTICS_CONTEXT, *PHANDLE_STATISTICS_CONTEXT;

INT_PTR CALLBACK PhpHandleStatisticsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowHandleStatisticsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    PHANDLE_STATISTICS_CONTEXT context;
    PSYSTEM_HANDLE_INFORMATION_EX handleInfo;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process", status, 0);
        return;
    }

    status = PhEnumHandlesGeneric(
        ProcessId,
        processHandle,
        !!PhCsEnableHandleSnapshot,
        &handleInfo
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(processHandle);
        PhShowStatus(ParentWindowHandle, L"Unable to enumerate process handles", status, 0);
        return;
    }

    context = PhAllocateZero(sizeof(HANDLE_STATISTICS_CONTEXT));
    context->ProcessId = ProcessId;
    context->ProcessHandle = processHandle;
    context->Handles = handleInfo;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_HANDLESTATS),
        ParentWindowHandle,
        PhpHandleStatisticsDlgProc,
        context
        );
}

void PhDestroyHandleStatistics(
    _In_ PHANDLE_STATISTICS_CONTEXT Context)
{
    for (ULONG i = 0; i < MAX_OBJECT_TYPE_NUMBER; i++)
    {
        if (Context->Entries[i].Name)
        {
            PhDereferenceObject(Context->Entries[i].Name);
        }
    }

    if (Context->Handles)
    {
        PhFree(Context->Handles);
    }

    if (Context->ProcessHandle)
    {
        NtClose(Context->ProcessHandle);
    }
}

static LONG NTAPI PhpTypeCountCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PHANDLE_STATISTICS_ENTRY entry1 = Item1;
    PHANDLE_STATISTICS_ENTRY entry2 = Item2;

    return uintcmp(entry1->Count, entry2->Count);
}

INT_PTR CALLBACK PhpHandleStatisticsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PHANDLE_STATISTICS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PHANDLE_STATISTICS_CONTEXT)lParam;
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
            HANDLE processId;
            ULONG_PTR i;

            PhSetApplicationWindowIcon(hwndDlg);

            processId = context->ProcessId;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Count");

            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, PhpTypeCountCompareFunction);
            PhLoadListViewColumnsFromSetting(SETTING_HANDLE_STATISTICS_LIST_VIEW_COLUMNS, context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(SETTING_HANDLE_STATISTICS_LIST_VIEW_SORT, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_HANDLE_STATISTICS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_HANDLE_STATISTICS_WINDOW_POSITION, SETTING_HANDLE_STATISTICS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            for (i = 0; i < context->Handles->NumberOfHandles; i++)
            {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo;
                PHANDLE_STATISTICS_ENTRY entry;

                handleInfo = &context->Handles->Handles[i];

                if (handleInfo->UniqueProcessId != processId)
                    continue;
                if (handleInfo->ObjectTypeIndex >= MAX_OBJECT_TYPE_NUMBER)
                    continue;

                entry = &context->Entries[handleInfo->ObjectTypeIndex];

                if (PhIsNullOrEmptyString(entry->Name))
                {
                    entry->Name = PhGetObjectTypeIndexName(handleInfo->ObjectTypeIndex);

                    if (PhIsNullOrEmptyString(entry->Name))
                    {
                        PPH_STRING typeName = NULL;

                        PhGetHandleInformation(
                            context->ProcessHandle,
                            handleInfo->HandleValue,
                            handleInfo->ObjectTypeIndex,
                            NULL,
                            &typeName,
                            NULL,
                            NULL
                            );

                        PhMoveReference(&entry->Name, typeName);
                    }
                }

                entry->Count++;
            }

            for (i = 0; i < MAX_OBJECT_TYPE_NUMBER; i++)
            {
                PHANDLE_STATISTICS_ENTRY entry;
                PPH_STRING unknownType;
                PPH_STRING countString;
                LONG lvItemIndex;

                entry = &context->Entries[i];

                if (entry->Count == 0)
                    continue;

                if (PhIsNullOrEmptyString(entry->Name))
                {
                    unknownType = PhFormatString(L"(unknown: %lu)", (ULONG)i);
                    lvItemIndex = PhAddListViewItem(
                        context->ListViewHandle,
                        MAXINT,
                        PhGetString(unknownType),
                        entry
                        );
                    PhDereferenceObject(unknownType);
                }
                else
                {
                    lvItemIndex = PhAddListViewItem(
                        context->ListViewHandle,
                        MAXINT,
                        PhGetString(entry->Name),
                        entry
                        );
                }

                countString = PhFormatUInt64(entry->Count, TRUE);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, PhGetString(countString));
                PhDereferenceObject(countString);
            }

            ExtendedListView_SortItems(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDestroyHandleStatistics(context);

            PhSaveListViewSortColumnsToSetting(SETTING_HANDLE_STATISTICS_LIST_VIEW_SORT, context->ListViewHandle);
            PhSaveListViewColumnsToSetting(SETTING_HANDLE_STATISTICS_LIST_VIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_HANDLE_STATISTICS_WINDOW_POSITION, SETTING_HANDLE_STATISTICS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
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
