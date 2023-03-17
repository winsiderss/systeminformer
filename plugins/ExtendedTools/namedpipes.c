/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2023
 *
 */

#include "exttools.h"

typedef struct _PIPE_ENUM_DIALOG_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewWndHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} PIPE_ENUM_DIALOG_CONTEXT, *PPIPE_ENUM_DIALOG_CONTEXT;

BOOLEAN NTAPI EtNamedPipeDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PhAddItemList(Context, PhCreateStringEx(Information->FileName, Information->FileNameLength));
    return TRUE;
}

VOID EtEnumerateNamedPipeDirectory(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    static PH_STRINGREF objectName = PH_STRINGREF_INIT(DEVICE_NAMED_PIPE);
    NTSTATUS status;
    HANDLE pipeDirectoryHandle;
    IO_STATUS_BLOCK isb;
    PPH_LIST pipeList;
    ULONG count = 0;

    ExtendedListView_SetRedraw(Context->ListViewWndHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewWndHandle);

    status = PhOpenFile(
        &pipeDirectoryHandle,
        &objectName,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return;

    pipeList = PhCreateList(1);
    PhEnumDirectoryFile(pipeDirectoryHandle, NULL, EtNamedPipeDirectoryCallback, pipeList);

    for (ULONG i = 0; i < pipeList->Count; i++)
    {
        PPH_STRING pipeName = pipeList->Items[i];
        WCHAR value[PH_PTR_STR_LEN_1];
        HANDLE pipeHandle;
        HANDLE processID;
        INT lvItemIndex;

        status = PhOpenFile(
            &pipeHandle,
            &pipeName->sr,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            pipeDirectoryHandle,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL
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

INT_PTR CALLBACK EtPipeEnumDlgProc(
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
        context->ParentWindowHandle = (HWND)lParam;

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
            context->ListViewWndHandle = GetDlgItem(hwndDlg, IDC_PIPELIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhSetListViewStyle(context->ListViewWndHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewWndHandle, L"explorer");
            PhAddListViewColumn(context->ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"PID");
            PhSetExtendedListView(context->ListViewWndHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, context->ListViewWndHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EtEnumerateNamedPipeDirectory(context);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

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
                EtEnumerateNamedPipeDirectory(context);
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

VOID EtShowPipeEnumDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PIPEDIALOG),
        NULL,
        EtPipeEnumDlgProc,
        ParentWindowHandle
        );
}
