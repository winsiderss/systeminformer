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
#include <secedit.h>
#include <kphuser.h>
#include <hndlinfo.h>

typedef struct _PIPE_ENUM_DIALOG_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewWndHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    BOOLEAN UseKph;
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
            HANDLE processID;
            FILE_PIPE_INFORMATION pipeInfo;
            FILE_PIPE_LOCAL_INFORMATION pipeLocalInfo;

            if (NT_SUCCESS(PhGetNamedPipeServerProcessId(pipeHandle, &processID)))
            {
                CLIENT_ID clientId;

                clientId.UniqueProcess = processID;
                clientId.UniqueThread = 0;

                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 2, PH_AUTO_T(PH_STRING, PhStdGetClientIdName(&clientId))->Buffer);
            }

            if (NT_SUCCESS(NtQueryInformationFile(pipeHandle, &isb, &pipeLocalInfo, sizeof(pipeLocalInfo), FilePipeLocalInformation)))
            {
                // Will always be client, since we opened the pipe by name. NtCreateNamedPipeFile must be used to create/open the server end.
                assert(pipeLocalInfo.NamedPipeEnd == FILE_PIPE_CLIENT_END);

                switch (pipeLocalInfo.NamedPipeType & ~FILE_PIPE_REJECT_REMOTE_CLIENTS)
                {
                case FILE_PIPE_BYTE_STREAM_TYPE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 3, L"Stream");
                    break;
                case FILE_PIPE_MESSAGE_TYPE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 3, L"Message");
                    break;
                }

                switch (pipeLocalInfo.NamedPipeConfiguration)
                {
                case FILE_PIPE_INBOUND:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 4, L"Inbound");
                    break;
                case FILE_PIPE_OUTBOUND:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 4, L"Outbound");
                    break;
                case FILE_PIPE_FULL_DUPLEX:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 4, L"Duplex");
                    break;
                }

                if (pipeLocalInfo.MaximumInstances == FILE_PIPE_UNLIMITED_INSTANCES)
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 5, L"Unlimited");
                else
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 5, PhaFormatUInt64(pipeLocalInfo.MaximumInstances, FALSE)->Buffer);
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 6, PhaFormatUInt64(pipeLocalInfo.CurrentInstances, FALSE)->Buffer);
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 7, PhaFormatSize(pipeLocalInfo.ReadDataAvailable, FALSE)->Buffer);
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 8, PhaFormatSize(pipeLocalInfo.OutboundQuota, FALSE)->Buffer);

                switch (pipeLocalInfo.NamedPipeState)
                {
                case FILE_PIPE_DISCONNECTED_STATE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 9, L"Disconnected");
                    break;
                case FILE_PIPE_LISTENING_STATE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 9, L"Listening");
                    break;
                case FILE_PIPE_CONNECTED_STATE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 9, L"Connected");
                    break;
                case FILE_PIPE_CLOSING_STATE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 9, L"Closing");
                    break;
                }

                if (pipeLocalInfo.NamedPipeType & FILE_PIPE_REJECT_REMOTE_CLIENTS)
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 10, L"Reject");
                else
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 10, L"Accept");
            }

            if (NT_SUCCESS(NtQueryInformationFile(pipeHandle, &isb, &pipeInfo, sizeof(pipeInfo), FilePipeInformation)))
            {
                switch (pipeInfo.ReadMode)
                {
                case FILE_PIPE_BYTE_STREAM_MODE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 11, L"Stream");
                    break;
                case FILE_PIPE_MESSAGE_MODE:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 11, L"Message");
                    break;
                }

                switch (pipeInfo.CompletionMode)
                {
                case FILE_PIPE_QUEUE_OPERATION:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 12, L"Queue");
                    break;
                case FILE_PIPE_COMPLETE_OPERATION:
                    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 12, L"Complete");
                    break;
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

VOID EtAddNamedPipeHandleToListView(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ PKPH_PROCESS_HANDLE HandleInfo,
    _In_ PPH_STRING PipeName
    )
{
    CLIENT_ID clientId;
    FILE_PIPE_INFORMATION pipeInfo;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInfo;
    INT lvItemIndex;
    WCHAR handle[PH_PTR_STR_LEN_1];
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;
    PPH_STRING accessString;
    PH_FORMAT format[4];
    WCHAR access[MAX_PATH];

    if (!NT_SUCCESS(PhCallKphQueryFileInformationWithTimeout(
        ProcessHandle,
        HandleInfo->Handle,
        FilePipeLocalInformation,
        &pipeLocalInfo,
        sizeof(pipeLocalInfo)
        )))
    {
        pipeLocalInfo.NamedPipeEnd = ULONG_MAX;
    }

    if (pipeLocalInfo.NamedPipeEnd == FILE_PIPE_CLIENT_END)
        lvItemIndex = PhAddListViewItem(Context->ListViewWndHandle, MAXINT, L"Client", NULL);
    else if (pipeLocalInfo.NamedPipeEnd == FILE_PIPE_SERVER_END)
        lvItemIndex = PhAddListViewItem(Context->ListViewWndHandle, MAXINT, L"Server", NULL);
    else
        lvItemIndex = PhAddListViewItem(Context->ListViewWndHandle, MAXINT, L"", NULL);

    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 1, PhGetString(PipeName));

    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = 0;
    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 2, PH_AUTO_T(PH_STRING, PhStdGetClientIdName(&clientId))->Buffer);

    PhPrintPointer(handle, HandleInfo->Handle);
    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 3, handle);

    if (PhGetAccessEntries(L"FileObject", &accessEntries, &numberOfAccessEntries))
        accessString = PhGetAccessString(HandleInfo->GrantedAccess, accessEntries, numberOfAccessEntries);
    else
        accessString = NULL;

    if (accessString)
    {
        PhInitFormatSR(&format[0], accessString->sr);
        PhInitFormatS(&format[1], L" (0x");
        PhInitFormatX(&format[2], HandleInfo->GrantedAccess);
        PhInitFormatS(&format[3], L" )");
        if (!PhFormatToBuffer(format, 4, access, sizeof(access), NULL))
            access[0] = UNICODE_NULL;
        PhDereferenceObject(accessString);
    }
    else
    {
        PhInitFormatS(&format[0], L"0x");
        PhInitFormatX(&format[1], HandleInfo->GrantedAccess);
        if (!PhFormatToBuffer(format, 2, access, sizeof(access), NULL))
            access[0] = UNICODE_NULL;
    }

    PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 4, access);

    if (pipeLocalInfo.NamedPipeEnd != ULONG_MAX)
    {
        switch (pipeLocalInfo.NamedPipeType & ~FILE_PIPE_REJECT_REMOTE_CLIENTS)
        {
            case FILE_PIPE_BYTE_STREAM_TYPE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 5, L"Stream");
                break;
            case FILE_PIPE_MESSAGE_TYPE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 5, L"Message");
                break;
        }

        switch (pipeLocalInfo.NamedPipeConfiguration)
        {
            case FILE_PIPE_INBOUND:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 6, L"Inbound");
                break;
            case FILE_PIPE_OUTBOUND:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 6, L"Outbound");
                break;
            case FILE_PIPE_FULL_DUPLEX:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 6, L"Duplex");
                break;
        }

        if (pipeLocalInfo.MaximumInstances == FILE_PIPE_UNLIMITED_INSTANCES)
            PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 7, L"Unlimited");
        else
            PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 7, PhaFormatUInt64(pipeLocalInfo.MaximumInstances, FALSE)->Buffer);

        PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 8, PhaFormatUInt64(pipeLocalInfo.CurrentInstances, FALSE)->Buffer);
        PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 9, PhaFormatSize(pipeLocalInfo.ReadDataAvailable, FALSE)->Buffer);
        PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 10, PhaFormatSize(pipeLocalInfo.OutboundQuota, FALSE)->Buffer);

        switch (pipeLocalInfo.NamedPipeState)
        {
            case FILE_PIPE_DISCONNECTED_STATE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 11, L"Disconnected");
                break;
            case FILE_PIPE_LISTENING_STATE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 11, L"Listening");
                break;
            case FILE_PIPE_CONNECTED_STATE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 11, L"Connected");
                break;
            case FILE_PIPE_CLOSING_STATE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 11, L"Closing");
                break;
        }

        if (pipeLocalInfo.NamedPipeType & FILE_PIPE_REJECT_REMOTE_CLIENTS)
            PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 12, L"Reject");
        else
            PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 12, L"Accept");
    }

    if (NT_SUCCESS(PhCallKphQueryFileInformationWithTimeout(
        ProcessHandle,
        HandleInfo->Handle,
        FilePipeInformation,
        &pipeInfo,
        sizeof(pipeInfo)
        )))
    {
        switch (pipeInfo.ReadMode)
        {
            case FILE_PIPE_BYTE_STREAM_MODE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 13, L"Stream");
                break;
            case FILE_PIPE_MESSAGE_MODE:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 13, L"Message");
                break;
        }

        switch (pipeInfo.CompletionMode)
        {
            case FILE_PIPE_QUEUE_OPERATION:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 14, L"Queue");
                break;
            case FILE_PIPE_COMPLETE_OPERATION:
                PhSetListViewSubItem(Context->ListViewWndHandle, lvItemIndex, 14, L"Complete");
                break;
        }
    }
}

VOID EtEnumerateNamedPipeHandles(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    ExtendedListView_SetRedraw(Context->ListViewWndHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewWndHandle);

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
    {
        return;
    }

    process = PH_FIRST_PROCESS(processes);
    do
    {
        HANDLE processHandle;
        PKPH_PROCESS_HANDLE_INFORMATION handles;

        if (!NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            process->UniqueProcessId
            )))
        {
            continue;
        }

        if (NT_SUCCESS(KphEnumerateProcessHandles2(processHandle, &handles)))
        {
            for (ULONG i = 0; i < handles->HandleCount; i++)
            {
                PKPH_PROCESS_HANDLE handle = &handles->Handles[i];
                DEVICE_TYPE deviceType;

                if (!NT_SUCCESS(PhGetDeviceType(processHandle, handle->Handle, &deviceType)))
                    continue;

                if (deviceType == FILE_DEVICE_NAMED_PIPE)
                {
                    PPH_STRING objectName;

                    if (!NT_SUCCESS(PhGetHandleInformation(
                        processHandle,
                        handle->Handle,
                        handle->ObjectTypeIndex,
                        NULL,
                        NULL,
                        NULL,
                        &objectName
                        )))
                    {
                        continue;
                    }

                    EtAddNamedPipeHandleToListView(
                        Context,
                        process->UniqueProcessId,
                        processHandle,
                        handle,
                        objectName
                        );
                }
            }

            PhFree(handles);
        }

        NtClose(processHandle);

    } while (process = PH_NEXT_PROCESS(process));

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
            context->UseKph = KphLevel() >= KphLevelMed;
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

            if (context->UseKph)
            {
                PhAddListViewColumn(context->ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"End");
                PhAddListViewColumn(context->ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
                PhAddListViewColumn(context->ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Process");
                PhAddListViewColumn(context->ListViewWndHandle, 3, 3, 3, LVCFMT_LEFT, 200, L"Handle");
                PhAddListViewColumn(context->ListViewWndHandle, 4, 4, 4, LVCFMT_LEFT, 50, L"Granted access");
                PhAddListViewColumn(context->ListViewWndHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Type");
                PhAddListViewColumn(context->ListViewWndHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Configuration");
                PhAddListViewColumn(context->ListViewWndHandle, 7, 7, 7, LVCFMT_LEFT, 80, L"Max instances");
                PhAddListViewColumn(context->ListViewWndHandle, 8, 8, 8, LVCFMT_LEFT, 80, L"Current instances");
                PhAddListViewColumn(context->ListViewWndHandle, 9, 9, 9, LVCFMT_LEFT, 80, L"Read data available");
                PhAddListViewColumn(context->ListViewWndHandle, 10, 10, 10, LVCFMT_LEFT, 80, L"Outbound quota");
                PhAddListViewColumn(context->ListViewWndHandle, 11, 11, 11, LVCFMT_LEFT, 80, L"State");
                PhAddListViewColumn(context->ListViewWndHandle, 12, 12, 12, LVCFMT_LEFT, 80, L"Remote clients");
                PhAddListViewColumn(context->ListViewWndHandle, 13, 13, 13, LVCFMT_LEFT, 80, L"Read mode");
                PhAddListViewColumn(context->ListViewWndHandle, 14, 14, 14, LVCFMT_LEFT, 80, L"Completion mode");
                PhSetExtendedListView(context->ListViewWndHandle);
                PhLoadListViewColumnsFromSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS_WITH_KPH, context->ListViewWndHandle);

                PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

                EtEnumerateNamedPipeHandles(context);
            }
            else
            {
                PhAddListViewColumn(context->ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
                PhAddListViewColumn(context->ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
                PhAddListViewColumn(context->ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"Server");
                PhAddListViewColumn(context->ListViewWndHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Type");
                PhAddListViewColumn(context->ListViewWndHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Configuration");
                PhAddListViewColumn(context->ListViewWndHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Max instances");
                PhAddListViewColumn(context->ListViewWndHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Current instances");
                PhAddListViewColumn(context->ListViewWndHandle, 7, 7, 7, LVCFMT_LEFT, 80, L"Read data available");
                PhAddListViewColumn(context->ListViewWndHandle, 8, 8, 8, LVCFMT_LEFT, 80, L"Outbound quota");
                PhAddListViewColumn(context->ListViewWndHandle, 9, 9, 9, LVCFMT_LEFT, 80, L"State");
                PhAddListViewColumn(context->ListViewWndHandle, 10, 10, 10, LVCFMT_LEFT, 80, L"Remote clients");
                PhAddListViewColumn(context->ListViewWndHandle, 11, 11, 11, LVCFMT_LEFT, 80, L"Read mode");
                PhAddListViewColumn(context->ListViewWndHandle, 12, 12, 12, LVCFMT_LEFT, 80, L"Completion mode");
                PhSetExtendedListView(context->ListViewWndHandle);
                PhLoadListViewColumnsFromSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, context->ListViewWndHandle);

                PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

                EtEnumerateNamedPipeDirectory(context);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, hwndDlg);
            if (context->UseKph)
                PhSaveListViewColumnsToSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS_WITH_KPH, context->ListViewWndHandle);
            else
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
                {
                    if (context->UseKph)
                        EtEnumerateNamedPipeHandles(context);
                    else
                        EtEnumerateNamedPipeDirectory(context);
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
