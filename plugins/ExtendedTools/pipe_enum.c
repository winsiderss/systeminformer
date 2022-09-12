/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2022
 *
 */

#include "exttools.h"

typedef struct _PIPE_ENUM_DIALOG_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewWndHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} PIPE_ENUM_DIALOG_CONTEXT, *PPIPE_ENUM_DIALOG_CONTEXT;

BOOLEAN NTAPI NamedPipeDirectoryCallback(
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PhAddItemList(Context, PhCreateStringEx(Information->FileName, Information->FileNameLength));
    return TRUE;
}

VOID EnumerateNamedPipeDirectory(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE pipeDirectoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectNameUs;
    IO_STATUS_BLOCK isb;
    PPH_LIST pipeList;
    ULONG count = 0;

    ExtendedListView_SetRedraw(Context->ListViewWndHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewWndHandle);

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
    PhEnumDirectoryFile(pipeDirectoryHandle, NULL, NamedPipeDirectoryCallback, pipeList);

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
        lvItemIndex = PhAddListViewItem(Context->ListViewWndHandle, MAXINT, value, NULL);
        PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 1, pipeName->Buffer);

        if (NT_SUCCESS(status))
        {
            FILE_PIPE_LOCAL_INFORMATION pipeInfo;

            if (NT_SUCCESS(NtQueryInformationFile(pipeHandle, &isb, &pipeInfo, sizeof(FILE_PIPE_LOCAL_INFORMATION), FilePipeLocalInformation)))
            {
                if (pipeInfo.NamedPipeEnd == FILE_PIPE_CLIENT_END)
                {
                    if (NT_SUCCESS(PhGetNamedPipeServerProcessId(pipeHandle, &processID)))
                    {
                        PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 2, PhaFormatUInt64(HandleToUlong(processID), FALSE)->Buffer);
                    }
                }
                else //if (pipeInfo.NamedPipeEnd == FILE_PIPE_SERVER_END)
                {
                    if (NT_SUCCESS(PhGetNamedPipeClientProcessId(pipeHandle, &processID)))
                    {
                        PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 2, PhaFormatUInt64(HandleToUlong(processID), FALSE)->Buffer);
                    }
                }
            }

            NtClose(pipeHandle);
        }

        PhDereferenceObject(pipeName);
    }

    PhDereferenceObject(pipeList);
    NtClose(pipeDirectoryHandle);

    ExtendedListView_SetRedraw(Context->ListViewWndHandle, TRUE);
}

INT_PTR CALLBACK PipeEnumDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPIPE_ENUM_DIALOG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PIPE_ENUM_DIALOG_CONTEXT));
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
            context->ListViewWndHandle = GetDlgItem(hwndDlg, IDC_ATOMLIST);

            PhCenterWindow(hwndDlg, PhMainWndHandle);
            PhSetApplicationWindowIcon(hwndDlg);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, hwndDlg);

            PhSetListViewStyle(context->ListViewWndHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewWndHandle, L"explorer");
            PhAddListViewColumn(context->ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"PID");
            PhSetExtendedListView(context->ListViewWndHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, context->ListViewWndHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EnumerateNamedPipeDirectory(context);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, hwndDlg);
            PhSaveListViewColumnsToSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, context->ListViewWndHandle);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
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
                EnumerateNamedPipeDirectory(context);
                break;
            }
        }
        break;
    }

    return FALSE;
}
