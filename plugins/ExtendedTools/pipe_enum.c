/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020
 *
 */

#include "exttools.h"

static HWND ListViewWndHandle;
static PH_LAYOUT_MANAGER LayoutManager;

BOOLEAN NTAPI PhpPipeFilenameDirectoryCallback(
    _In_ PVOID Information,
    _In_opt_ PVOID Context
)
{
    PFILE_DIRECTORY_INFORMATION info = Information;

    if (Context)
        PhAddItemList(Context, PhCreateStringEx(info->FileName, info->FileNameLength));
    return TRUE;
}

VOID EnumerateNamedPipeDirectory(VOID)
{
    NTSTATUS status;
    HANDLE pipeDirectoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectNameUs;
    IO_STATUS_BLOCK isb;
    PPH_LIST pipeList;
    ULONG count = 0;

    ExtendedListView_SetRedraw(ListViewWndHandle, FALSE);
    ListView_DeleteAllItems(ListViewWndHandle);

    RtlInitUnicodeString(&objectNameUs, DEVICE_NAMED_PIPE);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &pipeDirectoryHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return;

    pipeList = PhCreateList(1);
    PhEnumDirectoryFile(pipeDirectoryHandle, NULL, PhpPipeFilenameDirectoryCallback, pipeList);

    for (ULONG i = 0; i < pipeList->Count; i++)
    {
        PPH_STRING pipeName = pipeList->Items[i];
        WCHAR value[PH_PTR_STR_LEN_1];
        HANDLE pipeHandle;
        HANDLE processID;
        INT lvItemIndex;

        PhStringRefToUnicodeString(&pipeName->sr, &objectNameUs);
        InitializeObjectAttributes(
            &objectAttributes,
            &objectNameUs,
            OBJ_CASE_INSENSITIVE,
            pipeDirectoryHandle,
            NULL
            );

        status = NtOpenFile(
            &pipeHandle,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            &objectAttributes,
            &isb,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        PhPrintUInt32(value, ++count);
        lvItemIndex = PhAddListViewItem(ListViewWndHandle, MAXINT, value, NULL);
        PhSetListViewSubItem(ListViewWndHandle, lvItemIndex, 1, pipeName->Buffer);

        if (NT_SUCCESS(status))
        {
            FILE_PIPE_LOCAL_INFORMATION pipeInfo;

            if (NT_SUCCESS(NtQueryInformationFile(pipeHandle, &isb, &pipeInfo, sizeof(FILE_PIPE_LOCAL_INFORMATION), FilePipeLocalInformation)))
            {
                if (pipeInfo.NamedPipeEnd == FILE_PIPE_CLIENT_END)
                {
                    if (NT_SUCCESS(PhGetNamedPipeServerProcessId(pipeHandle, &processID)))
                    {
                        PhSetListViewSubItem(ListViewWndHandle, lvItemIndex, 2, PhaFormatUInt64(HandleToUlong(processID), FALSE)->Buffer);
                    }
                }
                else //if (pipeInfo.NamedPipeEnd == FILE_PIPE_SERVER_END)
                {
                    if (NT_SUCCESS(PhGetNamedPipeClientProcessId(pipeHandle, &processID)))
                    {
                        PhSetListViewSubItem(ListViewWndHandle, lvItemIndex, 2, PhaFormatUInt64(HandleToUlong(processID), FALSE)->Buffer);
                    }
                }
            }

            NtClose(pipeHandle);
        }

        PhDereferenceObject(pipeName);
    }

    PhDereferenceObject(pipeList);
    NtClose(pipeDirectoryHandle);

    ExtendedListView_SetRedraw(ListViewWndHandle, TRUE);
}

INT_PTR CALLBACK PipeEnumDlgProc(
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
            PhCenterWindow(hwndDlg, PhMainWndHandle);
            ListViewWndHandle = GetDlgItem(hwndDlg, IDC_ATOMLIST);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhLoadWindowPlacementFromSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, hwndDlg);

            PhSetListViewStyle(ListViewWndHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewWndHandle, L"explorer");
            PhAddListViewColumn(ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"PID");
            PhSetExtendedListView(ListViewWndHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, ListViewWndHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EnumerateNamedPipeDirectory();
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&LayoutManager);
        break;
    case WM_DESTROY:
        PhSaveWindowPlacementToSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, hwndDlg);
        PhSaveListViewColumnsToSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, ListViewWndHandle);
        PhDeleteLayoutManager(&LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDRETRY:
                EnumerateNamedPipeDirectory();
                break;
            }
        }
        break;
    }

    return FALSE;
}
