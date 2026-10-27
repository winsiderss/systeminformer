/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2026
 *
 */

#include "exttools.h"
#include <secedit.h>
#include <kphuser.h>
#include <hndlinfo.h>

#define ET_PIPE_ENUM_MAX_COLUMNS 15
#define ET_PIPE_NOTIFY_BUFFER_SIZE (16 * 1024)
#define WM_ET_PIPE_DIRECTORY_CHANGE (WM_APP + 1)

typedef enum _ET_PIPE_DIRECTORY_CHANGE_ACTION
{
    EtPipeDirectoryAdded,
    EtPipeDirectoryRemoved,
    EtPipeDirectoryResynchronize,
    EtPipeDirectoryError
} ET_PIPE_DIRECTORY_CHANGE_ACTION;

typedef struct _PIPE_ENUM_ITEM
{
    PPH_STRING Name;
    PPH_STRING Text[ET_PIPE_ENUM_MAX_COLUMNS];
} PIPE_ENUM_ITEM, *PPIPE_ENUM_ITEM;

typedef struct _PIPE_DIRECTORY_CHANGE
{
    SLIST_ENTRY ListEntry;
    ET_PIPE_DIRECTORY_CHANGE_ACTION Action;
    NTSTATUS Status;
    PPH_STRING Name;
} PIPE_DIRECTORY_CHANGE, *PPIPE_DIRECTORY_CHANGE;

typedef struct _PIPE_ENUM_DIALOG_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewWndHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LIST PipeList;
    HANDLE PipeDirectoryHandle;
    PTP_IO NotifyIo;
    IO_STATUS_BLOCK NotifyIoStatusBlock;
    SLIST_HEADER NotifyListHead;
    PH_QUEUED_LOCK NotifyLock;
    volatile LONG WatchEnabled;
    volatile LONG NotifyMessagePending;
    BOOLEAN UseKph;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) UCHAR NotifyBuffer[ET_PIPE_NOTIFY_BUFFER_SIZE];
} PIPE_ENUM_DIALOG_CONTEXT, *PPIPE_ENUM_DIALOG_CONTEXT;

VOID EtDestroyNamedPipeItem(
    _In_ PPIPE_ENUM_ITEM Item
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(Item->Text); i++)
        PhClearReference(&Item->Text[i]);

    PhClearReference(&Item->Name);
    PhFree(Item);
}

VOID EtDestroyNamedPipeList(
    _In_opt_ PPH_LIST PipeList
    )
{
    if (!PipeList)
        return;

    for (ULONG i = 0; i < PipeList->Count; i++)
        EtDestroyNamedPipeItem(PipeList->Items[i]);

    PhDereferenceObject(PipeList);
}

PPIPE_ENUM_ITEM EtCreateNamedPipeItem(
    _In_ PPH_STRING Name
    )
{
    PPIPE_ENUM_ITEM item;

    item = PhAllocateZero(sizeof(PIPE_ENUM_ITEM));
    item->Name = PhReferenceObject(Name);

    return item;
}

VOID EtSetNamedPipeItemText(
    _Inout_ PPIPE_ENUM_ITEM Item,
    _In_ ULONG SubItem,
    _In_opt_ PCWSTR Text
    )
{
    if (SubItem >= RTL_NUMBER_OF(Item->Text))
        return;

    PhMoveReference(&Item->Text[SubItem], PhCreateString(Text ? Text : L""));
}

VOID EtReplaceNamedPipeList(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context,
    _In_ PPH_LIST PipeList
    )
{
    PPH_LIST oldPipeList;

    ExtendedListView_SetRedraw(Context->ListViewWndHandle, FALSE);
    ListView_SetItemState(
        Context->ListViewWndHandle,
        -1,
        0,
        LVIS_FOCUSED | LVIS_SELECTED
        );

    oldPipeList = Context->PipeList;
    Context->PipeList = PipeList;

    ListView_SetItemCountEx(
        Context->ListViewWndHandle,
        PipeList->Count,
        LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL
        );
    InvalidateRect(Context->ListViewWndHandle, NULL, FALSE);

    EtDestroyNamedPipeList(oldPipeList);
    ExtendedListView_SetRedraw(Context->ListViewWndHandle, TRUE);
}

NTSTATUS EtOpenNamedPipeDirectory(
    _Out_ PHANDLE PipeDirectoryHandle,
    _In_ BOOLEAN Asynchronous
    )
{
    static CONST PH_STRINGREF objectName = PH_STRINGREF_INIT(DEVICE_NAMED_PIPE);

    return PhOpenFile(
        PipeDirectoryHandle,
        &objectName,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | (Asynchronous ? 0 : FILE_SYNCHRONOUS_IO_NONALERT),
        NULL
        );
}

_Function_class_(PH_ENUM_DIRECTORY_FILE)
BOOLEAN NTAPI EtNamedPipeDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PhAddItemList(Context, PhCreateStringEx(Information->FileName, Information->FileNameLength));
    return TRUE;
}

PPIPE_ENUM_ITEM EtCreateNamedPipeDirectoryItem(
    _In_ HANDLE PipeDirectoryHandle,
    _In_ PPH_STRING PipeName
    )
{
    NTSTATUS status;
    PPIPE_ENUM_ITEM item;
    HANDLE pipeHandle;
    IO_STATUS_BLOCK isb;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    SECURITY_QUALITY_OF_SERVICE pipeSecurityQos =
    {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_STATIC_TRACKING,
        FALSE
    };

    item = EtCreateNamedPipeItem(PipeName);
    EtSetNamedPipeItemText(item, 1, PipeName->Buffer);

    if (!PhStringRefToUnicodeString(&PipeName->sr, &fileName))
        return item;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        PipeDirectoryHandle,
        NULL
        );

    objectAttributes.SecurityQualityOfService = &pipeSecurityQos;

    status = NtOpenFile(
        &pipeHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

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

            EtSetNamedPipeItemText(item, 2, PH_AUTO_T(PH_STRING, PhStdGetClientIdName(&clientId))->Buffer);
        }

        if (NT_SUCCESS(NtQueryInformationFile(pipeHandle, &isb, &pipeLocalInfo, sizeof(pipeLocalInfo), FilePipeLocalInformation)))
        {
            // Will always be client, since we opened the pipe by name. NtCreateNamedPipeFile must be used to create/open the server end.
            assert(pipeLocalInfo.NamedPipeEnd == FILE_PIPE_CLIENT_END);

            switch (pipeLocalInfo.NamedPipeType & ~FILE_PIPE_REJECT_REMOTE_CLIENTS)
            {
            case FILE_PIPE_BYTE_STREAM_TYPE:
                EtSetNamedPipeItemText(item, 3, L"Stream");
                break;
            case FILE_PIPE_MESSAGE_TYPE:
                EtSetNamedPipeItemText(item, 3, L"Message");
                break;
            }

            switch (pipeLocalInfo.NamedPipeConfiguration)
            {
            case FILE_PIPE_INBOUND:
                EtSetNamedPipeItemText(item, 4, L"Inbound");
                break;
            case FILE_PIPE_OUTBOUND:
                EtSetNamedPipeItemText(item, 4, L"Outbound");
                break;
            case FILE_PIPE_FULL_DUPLEX:
                EtSetNamedPipeItemText(item, 4, L"Duplex");
                break;
            }

            if (pipeLocalInfo.MaximumInstances == FILE_PIPE_UNLIMITED_INSTANCES)
                EtSetNamedPipeItemText(item, 5, L"Unlimited");
            else
                EtSetNamedPipeItemText(item, 5, PhaFormatUInt64(pipeLocalInfo.MaximumInstances, FALSE)->Buffer);
            EtSetNamedPipeItemText(item, 6, PhaFormatUInt64(pipeLocalInfo.CurrentInstances, FALSE)->Buffer);
            EtSetNamedPipeItemText(item, 7, PhaFormatSize(pipeLocalInfo.ReadDataAvailable, FALSE)->Buffer);
            EtSetNamedPipeItemText(item, 8, PhaFormatSize(pipeLocalInfo.OutboundQuota, FALSE)->Buffer);

            switch (pipeLocalInfo.NamedPipeState)
            {
            case FILE_PIPE_DISCONNECTED_STATE:
                EtSetNamedPipeItemText(item, 9, L"Disconnected");
                break;
            case FILE_PIPE_LISTENING_STATE:
                EtSetNamedPipeItemText(item, 9, L"Listening");
                break;
            case FILE_PIPE_CONNECTED_STATE:
                EtSetNamedPipeItemText(item, 9, L"Connected");
                break;
            case FILE_PIPE_CLOSING_STATE:
                EtSetNamedPipeItemText(item, 9, L"Closing");
                break;
            }

            if (pipeLocalInfo.NamedPipeType & FILE_PIPE_REJECT_REMOTE_CLIENTS)
                EtSetNamedPipeItemText(item, 10, L"Reject");
            else
                EtSetNamedPipeItemText(item, 10, L"Accept");
        }

        if (NT_SUCCESS(NtQueryInformationFile(pipeHandle, &isb, &pipeInfo, sizeof(pipeInfo), FilePipeInformation)))
        {
            switch (pipeInfo.ReadMode)
            {
            case FILE_PIPE_BYTE_STREAM_MODE:
                EtSetNamedPipeItemText(item, 11, L"Stream");
                break;
            case FILE_PIPE_MESSAGE_MODE:
                EtSetNamedPipeItemText(item, 11, L"Message");
                break;
            }

            switch (pipeInfo.CompletionMode)
            {
            case FILE_PIPE_QUEUE_OPERATION:
                EtSetNamedPipeItemText(item, 12, L"Queue");
                break;
            case FILE_PIPE_COMPLETE_OPERATION:
                EtSetNamedPipeItemText(item, 12, L"Complete");
                break;
            }
        }

        NtClose(pipeHandle);
    }

    return item;
}

VOID EtEnumerateNamedPipeDirectory(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE pipeDirectoryHandle;
    PPH_LIST pipeNames;
    PPH_LIST pipeList;

    status = EtOpenNamedPipeDirectory(&pipeDirectoryHandle, FALSE);
    if (!NT_SUCCESS(status))
        return;

    pipeNames = PhCreateList(16);
    pipeList = PhCreateList(16);
    status = PhEnumDirectoryFile(pipeDirectoryHandle, NULL, EtNamedPipeDirectoryCallback, pipeNames);

    if (NT_SUCCESS(status))
    {
        for (ULONG i = 0; i < pipeNames->Count; i++)
        {
            PPH_STRING pipeName = pipeNames->Items[i];

            PhAddItemList(pipeList, EtCreateNamedPipeDirectoryItem(pipeDirectoryHandle, pipeName));
        }

        EtReplaceNamedPipeList(Context, pipeList);
        pipeList = NULL;
    }

    for (ULONG i = 0; i < pipeNames->Count; i++)
        PhDereferenceObject(pipeNames->Items[i]);

    PhDereferenceObject(pipeNames);
    EtDestroyNamedPipeList(pipeList);
    NtClose(pipeDirectoryHandle);
}

PPIPE_ENUM_ITEM EtCreateNamedPipeHandleItem(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ PKPH_PROCESS_HANDLE HandleInfo,
    _In_ PPH_STRING PipeName
    )
{
    CLIENT_ID clientId;
    FILE_PIPE_INFORMATION pipeInfo;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInfo;
    PPIPE_ENUM_ITEM item;
    WCHAR handle[PH_PTR_STR_LEN_1];
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;
    PPH_STRING accessString;
    PH_FORMAT format[4];
    WCHAR access[MAX_PATH];

    item = EtCreateNamedPipeItem(PipeName);

    if (!NT_SUCCESS(PhCallKphQueryFileInformationWithTimeout(
        ProcessHandle,
        HandleInfo->Handle,
        FilePipeLocalInformation,
        &pipeLocalInfo,
        sizeof(pipeLocalInfo),
        NULL
        )))
    {
        pipeLocalInfo.NamedPipeEnd = ULONG_MAX;
    }

    if (pipeLocalInfo.NamedPipeEnd == FILE_PIPE_CLIENT_END)
        EtSetNamedPipeItemText(item, 0, L"Client");
    else if (pipeLocalInfo.NamedPipeEnd == FILE_PIPE_SERVER_END)
        EtSetNamedPipeItemText(item, 0, L"Server");
    else
        EtSetNamedPipeItemText(item, 0, L"");

    EtSetNamedPipeItemText(item, 1, PhGetString(PipeName));

    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = 0;
    EtSetNamedPipeItemText(item, 2, PH_AUTO_T(PH_STRING, PhStdGetClientIdName(&clientId))->Buffer);

    PhPrintPointer(handle, HandleInfo->Handle);
    EtSetNamedPipeItemText(item, 3, handle);

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

    EtSetNamedPipeItemText(item, 4, access);

    if (pipeLocalInfo.NamedPipeEnd != ULONG_MAX)
    {
        switch (pipeLocalInfo.NamedPipeType & ~FILE_PIPE_REJECT_REMOTE_CLIENTS)
        {
            case FILE_PIPE_BYTE_STREAM_TYPE:
                EtSetNamedPipeItemText(item, 5, L"Stream");
                break;
            case FILE_PIPE_MESSAGE_TYPE:
                EtSetNamedPipeItemText(item, 5, L"Message");
                break;
        }

        switch (pipeLocalInfo.NamedPipeConfiguration)
        {
            case FILE_PIPE_INBOUND:
                EtSetNamedPipeItemText(item, 6, L"Inbound");
                break;
            case FILE_PIPE_OUTBOUND:
                EtSetNamedPipeItemText(item, 6, L"Outbound");
                break;
            case FILE_PIPE_FULL_DUPLEX:
                EtSetNamedPipeItemText(item, 6, L"Duplex");
                break;
        }

        if (pipeLocalInfo.MaximumInstances == FILE_PIPE_UNLIMITED_INSTANCES)
            EtSetNamedPipeItemText(item, 7, L"Unlimited");
        else
            EtSetNamedPipeItemText(item, 7, PhaFormatUInt64(pipeLocalInfo.MaximumInstances, FALSE)->Buffer);

        EtSetNamedPipeItemText(item, 8, PhaFormatUInt64(pipeLocalInfo.CurrentInstances, FALSE)->Buffer);
        EtSetNamedPipeItemText(item, 9, PhaFormatSize(pipeLocalInfo.ReadDataAvailable, FALSE)->Buffer);
        EtSetNamedPipeItemText(item, 10, PhaFormatSize(pipeLocalInfo.OutboundQuota, FALSE)->Buffer);

        switch (pipeLocalInfo.NamedPipeState)
        {
            case FILE_PIPE_DISCONNECTED_STATE:
                EtSetNamedPipeItemText(item, 11, L"Disconnected");
                break;
            case FILE_PIPE_LISTENING_STATE:
                EtSetNamedPipeItemText(item, 11, L"Listening");
                break;
            case FILE_PIPE_CONNECTED_STATE:
                EtSetNamedPipeItemText(item, 11, L"Connected");
                break;
            case FILE_PIPE_CLOSING_STATE:
                EtSetNamedPipeItemText(item, 11, L"Closing");
                break;
        }

        if (pipeLocalInfo.NamedPipeType & FILE_PIPE_REJECT_REMOTE_CLIENTS)
            EtSetNamedPipeItemText(item, 12, L"Reject");
        else
            EtSetNamedPipeItemText(item, 12, L"Accept");
    }

    if (NT_SUCCESS(PhCallKphQueryFileInformationWithTimeout(
        ProcessHandle,
        HandleInfo->Handle,
        FilePipeInformation,
        &pipeInfo,
        sizeof(pipeInfo),
        NULL
        )))
    {
        switch (pipeInfo.ReadMode)
        {
            case FILE_PIPE_BYTE_STREAM_MODE:
                EtSetNamedPipeItemText(item, 13, L"Stream");
                break;
            case FILE_PIPE_MESSAGE_MODE:
                EtSetNamedPipeItemText(item, 13, L"Message");
                break;
        }

        switch (pipeInfo.CompletionMode)
        {
            case FILE_PIPE_QUEUE_OPERATION:
                EtSetNamedPipeItemText(item, 14, L"Queue");
                break;
            case FILE_PIPE_COMPLETE_OPERATION:
                EtSetNamedPipeItemText(item, 14, L"Complete");
                break;
        }
    }

    return item;
}

VOID EtEnumerateNamedPipeHandles(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_LIST pipeList;

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    pipeList = PhCreateList(64);

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

        if (NT_SUCCESS(KsiEnumerateProcessHandles(processHandle, &handles)))
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

                    PhAddItemList(pipeList, EtCreateNamedPipeHandleItem(
                        process->UniqueProcessId,
                        processHandle,
                        handle,
                        objectName
                        ));
                    PhDereferenceObject(objectName);
                }
            }

            PhFree(handles);
        }

        NtClose(processHandle);

    } while (process = PH_NEXT_PROCESS(process));

    PhFree(processes);
    EtReplaceNamedPipeList(Context, pipeList);
}

ULONG EtFindNamedPipeItem(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    for (ULONG i = 0; i < Context->PipeList->Count; i++)
    {
        PPIPE_ENUM_ITEM item = Context->PipeList->Items[i];

        if (PhEqualString(item->Name, Name, TRUE))
            return i;
    }

    return ULONG_MAX;
}

BOOLEAN EtAddNamedPipeDirectoryModelItem(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    NTSTATUS status;
    HANDLE pipeDirectoryHandle;
    BOOLEAN closePipeDirectoryHandle;

    if (EtFindNamedPipeItem(Context, Name) != ULONG_MAX)
        return FALSE;

    if (Context->PipeDirectoryHandle)
    {
        pipeDirectoryHandle = Context->PipeDirectoryHandle;
        closePipeDirectoryHandle = FALSE;
    }
    else
    {
        status = EtOpenNamedPipeDirectory(&pipeDirectoryHandle, FALSE);
        if (!NT_SUCCESS(status))
            return FALSE;

        closePipeDirectoryHandle = TRUE;
    }

    PhAddItemList(
        Context->PipeList,
        EtCreateNamedPipeDirectoryItem(pipeDirectoryHandle, Name)
        );

    if (closePipeDirectoryHandle)
        NtClose(pipeDirectoryHandle);

    return TRUE;
}

BOOLEAN EtRemoveNamedPipeDirectoryModelItem(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context,
    _In_ PPH_STRING Name,
    _Out_ PULONG RemovedIndex
    )
{
    ULONG index;
    PPIPE_ENUM_ITEM item;

    index = EtFindNamedPipeItem(Context, Name);
    if (index == ULONG_MAX)
        return FALSE;

    item = Context->PipeList->Items[index];
    PhRemoveItemList(Context->PipeList, index);
    EtDestroyNamedPipeItem(item);

    *RemovedIndex = index;

    return TRUE;
}

VOID EtFreeNamedPipeDirectoryChanges(
    _In_opt_ PSLIST_ENTRY ListEntry
    )
{
    while (ListEntry)
    {
        PPIPE_DIRECTORY_CHANGE change;

        change = CONTAINING_RECORD(ListEntry, PIPE_DIRECTORY_CHANGE, ListEntry);
        ListEntry = ListEntry->Next;

        PhClearReference(&change->Name);
        PhFree(change);
    }
}

VOID EtQueueNamedPipeDirectoryChange(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context,
    _In_ ET_PIPE_DIRECTORY_CHANGE_ACTION Action,
    _In_ NTSTATUS Status,
    _In_opt_ PPH_STRING Name
    )
{
    PPIPE_DIRECTORY_CHANGE change;

    change = PhAllocateZero(sizeof(PIPE_DIRECTORY_CHANGE));
    change->Action = Action;
    change->Status = Status;

    if (Name)
        change->Name = PhReferenceObject(Name);

    RtlInterlockedPushEntrySList(&Context->NotifyListHead, &change->ListEntry);

    if (_InterlockedExchange(&Context->NotifyMessagePending, 1) == 0)
    {
        if (!PostMessage(Context->WindowHandle, WM_ET_PIPE_DIRECTORY_CHANGE, 0, 0))
            _InterlockedExchange(&Context->NotifyMessagePending, 0);
    }
}

NTSTATUS EtArmNamedPipeDirectoryWatcher(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;

    PhAcquireQueuedLockExclusive(&Context->NotifyLock);

    if (
        _InterlockedCompareExchange(&Context->WatchEnabled, 0, 0) == 0 ||
        !Context->NotifyIo ||
        !Context->PipeDirectoryHandle
        )
    {
        status = STATUS_CANCELLED;
        goto CleanupExit;
    }

    RtlZeroMemory(&Context->NotifyIoStatusBlock, sizeof(IO_STATUS_BLOCK));
    TpStartAsyncIoOperation(Context->NotifyIo);

    status = NtNotifyChangeDirectoryFile(
        Context->PipeDirectoryHandle,
        NULL,
        NULL,
        &Context->NotifyIoStatusBlock,
        &Context->NotifyIoStatusBlock,
        Context->NotifyBuffer,
        sizeof(Context->NotifyBuffer),
        FILE_NOTIFY_CHANGE_FILE_NAME,
        FALSE
        );

    if (!NT_SUCCESS(status))
        TpCancelAsyncIoOperation(Context->NotifyIo);

CleanupExit:

    PhReleaseQueuedLockExclusive(&Context->NotifyLock);

    return status;
}

_Function_class_(TP_IO_CALLBACK)
VOID NTAPI EtNamedPipeDirectoryIoCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Parameter,
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ PTP_IO Io
    )
{
    PPIPE_ENUM_DIALOG_CONTEXT context;
    BOOLEAN queueResynchronize;

    context = Parameter;
    queueResynchronize = FALSE;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(ApcContext);
    UNREFERENCED_PARAMETER(Io);

    if (_InterlockedCompareExchange(&context->WatchEnabled, 0, 0) == 0)
        return;

    if (IoStatusBlock->Status == STATUS_SUCCESS && IoStatusBlock->Information != 0)
    {
        ULONG offset;
        ULONG length;

        offset = 0;
        length = (ULONG)min(IoStatusBlock->Information, sizeof(context->NotifyBuffer));

        while (offset < length)
        {
            PFILE_NOTIFY_INFORMATION information;
            ET_PIPE_DIRECTORY_CHANGE_ACTION action;
            PPH_STRING name;

            if (length - offset < (ULONG)FIELD_OFFSET(FILE_NOTIFY_INFORMATION, FileName))
            {
                queueResynchronize = TRUE;
                break;
            }

            information = PTR_ADD_OFFSET(context->NotifyBuffer, offset);

            if (
                information->FileNameLength > length - offset - FIELD_OFFSET(FILE_NOTIFY_INFORMATION, FileName) ||
                information->FileNameLength % sizeof(WCHAR) != 0
                )
            {
                queueResynchronize = TRUE;
                break;
            }

            switch (information->Action)
            {
            case FILE_ACTION_ADDED:
            case FILE_ACTION_RENAMED_NEW_NAME:
                action = EtPipeDirectoryAdded;
                break;
            case FILE_ACTION_REMOVED:
            case FILE_ACTION_RENAMED_OLD_NAME:
                action = EtPipeDirectoryRemoved;
                break;
            default:
                action = EtPipeDirectoryResynchronize;
                queueResynchronize = TRUE;
                break;
            }

            if (action != EtPipeDirectoryResynchronize)
            {
                name = PhCreateStringEx(information->FileName, information->FileNameLength);
                EtQueueNamedPipeDirectoryChange(context, action, STATUS_SUCCESS, name);
                PhDereferenceObject(name);
            }

            if (information->NextEntryOffset == 0)
                break;

            if (
                information->NextEntryOffset < (ULONG)FIELD_OFFSET(FILE_NOTIFY_INFORMATION, FileName) ||
                information->NextEntryOffset > length - offset ||
                !IS_ALIGNED(information->NextEntryOffset, sizeof(ULONG))
                )
            {
                queueResynchronize = TRUE;
                break;
            }

            offset += information->NextEntryOffset;
        }
    }
    else if (IoStatusBlock->Status == STATUS_SUCCESS)
    {
        queueResynchronize = TRUE;
    }
    else if (IoStatusBlock->Status != STATUS_CANCELLED)
    {
        queueResynchronize = TRUE;
    }

    if (queueResynchronize)
        EtQueueNamedPipeDirectoryChange(context, EtPipeDirectoryResynchronize, IoStatusBlock->Status, NULL);

    if (_InterlockedCompareExchange(&context->WatchEnabled, 0, 0) != 0)
    {
        NTSTATUS status;

        status = EtArmNamedPipeDirectoryWatcher(context);

        if (!NT_SUCCESS(status) && status != STATUS_CANCELLED)
            EtQueueNamedPipeDirectoryChange(context, EtPipeDirectoryError, status, NULL);
    }
}

VOID EtStopNamedPipeDirectoryWatcher(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    IO_STATUS_BLOCK ioStatusBlock;
    PTP_IO notifyIo;

    _InterlockedExchange(&Context->WatchEnabled, 0);
    PhAcquireQueuedLockExclusive(&Context->NotifyLock);

    if (Context->NotifyIo)
    {
        if (Context->PipeDirectoryHandle)
        {
            NtCancelIoFileEx(
                Context->PipeDirectoryHandle,
                &Context->NotifyIoStatusBlock,
                &ioStatusBlock
                );
        }

        notifyIo = Context->NotifyIo;
        Context->NotifyIo = NULL;
    }
    else
    {
        notifyIo = NULL;
    }

    PhReleaseQueuedLockExclusive(&Context->NotifyLock);

    if (notifyIo)
    {
        TpWaitForIoCompletion(notifyIo, TRUE);
        TpReleaseIoCompletion(notifyIo);
    }

    if (Context->PipeDirectoryHandle)
    {
        NtClose(Context->PipeDirectoryHandle);
        Context->PipeDirectoryHandle = NULL;
    }

    _InterlockedExchange(&Context->NotifyMessagePending, 0);
    EtFreeNamedPipeDirectoryChanges(RtlInterlockedFlushSList(&Context->NotifyListHead));
}

NTSTATUS EtStartNamedPipeDirectoryWatcher(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;

    if (_InterlockedCompareExchange(&Context->WatchEnabled, 0, 0) != 0)
        return STATUS_SUCCESS;

    status = EtOpenNamedPipeDirectory(&Context->PipeDirectoryHandle, TRUE);
    if (!NT_SUCCESS(status))
        return status;

    status = TpAllocIoCompletion(
        &Context->NotifyIo,
        Context->PipeDirectoryHandle,
        EtNamedPipeDirectoryIoCallback,
        Context,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(Context->PipeDirectoryHandle);
        Context->PipeDirectoryHandle = NULL;
        return status;
    }

    _InterlockedExchange(&Context->WatchEnabled, 1);
    status = EtArmNamedPipeDirectoryWatcher(Context);

    if (!NT_SUCCESS(status))
        EtStopNamedPipeDirectoryWatcher(Context);

    return status;
}

PSLIST_ENTRY EtReverseNamedPipeDirectoryChanges(
    _In_opt_ PSLIST_ENTRY ListEntry
    )
{
    PSLIST_ENTRY previousEntry;

    previousEntry = NULL;

    while (ListEntry)
    {
        PSLIST_ENTRY nextEntry;

        nextEntry = ListEntry->Next;
        ListEntry->Next = previousEntry;
        previousEntry = ListEntry;
        ListEntry = nextEntry;
    }

    return previousEntry;
}

VOID EtProcessNamedPipeDirectoryChanges(
    _In_ PPIPE_ENUM_DIALOG_CONTEXT Context
    )
{
    PSLIST_ENTRY listEntry;
    BOOLEAN changed;
    BOOLEAN resynchronize;
    NTSTATUS errorStatus;
    ULONG firstChangedIndex;

    _InterlockedExchange(&Context->NotifyMessagePending, 0);
    listEntry = EtReverseNamedPipeDirectoryChanges(
        RtlInterlockedFlushSList(&Context->NotifyListHead)
        );
    changed = FALSE;
    resynchronize = FALSE;
    errorStatus = STATUS_SUCCESS;
    firstChangedIndex = ULONG_MAX;

    while (listEntry)
    {
        PPIPE_DIRECTORY_CHANGE change;
        PSLIST_ENTRY nextEntry;

        nextEntry = listEntry->Next;
        change = CONTAINING_RECORD(listEntry, PIPE_DIRECTORY_CHANGE, ListEntry);

        if (Context->UseKph)
            resynchronize = TRUE;

        switch (change->Action)
        {
        case EtPipeDirectoryAdded:
            if (!resynchronize && EtAddNamedPipeDirectoryModelItem(Context, change->Name))
            {
                changed = TRUE;
                firstChangedIndex = min(firstChangedIndex, Context->PipeList->Count - 1);
            }
            break;
        case EtPipeDirectoryRemoved:
            if (!resynchronize)
            {
                ULONG removedIndex;

                if (EtRemoveNamedPipeDirectoryModelItem(Context, change->Name, &removedIndex))
                {
                    changed = TRUE;
                    firstChangedIndex = min(firstChangedIndex, removedIndex);
                }
            }
            break;
        case EtPipeDirectoryResynchronize:
            resynchronize = TRUE;
            break;
        case EtPipeDirectoryError:
            errorStatus = change->Status;
            break;
        }

        PhClearReference(&change->Name);
        PhFree(change);
        listEntry = nextEntry;
    }

    if (!NT_SUCCESS(errorStatus))
    {
        EtStopNamedPipeDirectoryWatcher(Context);
        Button_SetCheck(GetDlgItem(Context->WindowHandle, IDC_PIPE_WATCH), BST_UNCHECKED);
        PhSetIntegerSetting(SETTING_NAME_PIPE_ENUM_WATCH_CHANGES, FALSE);
        PhShowStatus(Context->WindowHandle, L"Unable to monitor the named pipe directory", errorStatus, 0);
        resynchronize = TRUE;
    }

    if (resynchronize)
    {
        if (Context->UseKph)
            EtEnumerateNamedPipeHandles(Context);
        else
            EtEnumerateNamedPipeDirectory(Context);
    }
    else if (changed)
    {
        ListView_SetItemCountEx(
            Context->ListViewWndHandle,
            Context->PipeList->Count,
            LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL
            );

        if (Context->PipeList->Count != 0 && firstChangedIndex != ULONG_MAX)
        {
            ListView_RedrawItems(
                Context->ListViewWndHandle,
                firstChangedIndex,
                Context->PipeList->Count - 1
                );
        }
    }
}

INT_PTR CALLBACK EtPipeEnumDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPIPE_ENUM_DIALOG_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PIPE_ENUM_DIALOG_CONTEXT));
        context->WindowHandle = WindowHandle;
        context->ParentWindowHandle = (HWND)lParam;
        context->PipeList = PhCreateList(64);
        PhInitializeSListHead(&context->NotifyListHead);
        PhInitializeQueuedLock(&context->NotifyLock);

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->UseKph = KsiLevel() >= KphLevelMed;
            context->ListViewWndHandle = GetDlgItem(WindowHandle, IDC_PIPELIST);

            PhSetApplicationWindowIcon(WindowHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_PIPE_WATCH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

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
                PhLoadListViewColumnsFromSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS_WITH_KSI, context->ListViewWndHandle);

                PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
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

                PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
            }

            if (PhGetIntegerSetting(SETTING_NAME_PIPE_ENUM_WATCH_CHANGES))
            {
                NTSTATUS status;

                status = EtStartNamedPipeDirectoryWatcher(context);

                if (NT_SUCCESS(status))
                {
                    Button_SetCheck(GetDlgItem(WindowHandle, IDC_PIPE_WATCH), BST_CHECKED);
                }
                else
                {
                    PhSetIntegerSetting(SETTING_NAME_PIPE_ENUM_WATCH_CHANGES, FALSE);
                    PhShowStatus(WindowHandle, L"Unable to monitor the named pipe directory", status, 0);
                }
            }

            if (context->UseKph)
                EtEnumerateNamedPipeHandles(context);
            else
                EtEnumerateNamedPipeDirectory(context);
        }
        break;
    case WM_DESTROY:
        {
            EtStopNamedPipeDirectoryWatcher(context);
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, WindowHandle);
            if (context->UseKph)
                PhSaveListViewColumnsToSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS_WITH_KSI, context->ListViewWndHandle);
            else
                PhSaveListViewColumnsToSetting(SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, context->ListViewWndHandle);

            PhDeleteLayoutManager(&context->LayoutManager);
            EtDestroyNamedPipeList(context->PipeList);
            PhFree(context);
        }
        break;
    case WM_ET_PIPE_DIRECTORY_CHANGE:
        {
            EtProcessNamedPipeDirectoryChanges(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(WindowHandle, IDOK);
                break;
            case IDRETRY:
                {
                    if (context->UseKph)
                        EtEnumerateNamedPipeHandles(context);
                    else
                        EtEnumerateNamedPipeDirectory(context);
                }
                break;
            case IDC_PIPE_WATCH:
                {
                    if (Button_GetCheck(GetDlgItem(WindowHandle, IDC_PIPE_WATCH)) == BST_CHECKED)
                    {
                        NTSTATUS status;

                        status = EtStartNamedPipeDirectoryWatcher(context);

                        if (NT_SUCCESS(status))
                        {
                            PhSetIntegerSetting(SETTING_NAME_PIPE_ENUM_WATCH_CHANGES, TRUE);
                        }
                        else
                        {
                            Button_SetCheck(GetDlgItem(WindowHandle, IDC_PIPE_WATCH), BST_UNCHECKED);
                            PhSetIntegerSetting(SETTING_NAME_PIPE_ENUM_WATCH_CHANGES, FALSE);
                            PhShowStatus(WindowHandle, L"Unable to monitor the named pipe directory", status, 0);
                        }
                    }
                    else
                    {
                        EtStopNamedPipeDirectoryWatcher(context);
                        PhSetIntegerSetting(SETTING_NAME_PIPE_ENUM_WATCH_CHANGES, FALSE);
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewWndHandle);

            if (header->hwndFrom == context->ListViewWndHandle && header->code == LVN_GETDISPINFO)
            {
                NMLVDISPINFO *dispInfo = (NMLVDISPINFO *)header;

                if (
                    FlagOn(dispInfo->item.mask, LVIF_TEXT) &&
                    dispInfo->item.iItem >= 0 &&
                    (ULONG)dispInfo->item.iItem < context->PipeList->Count
                    )
                {
                    PPIPE_ENUM_ITEM item;

                    item = context->PipeList->Items[dispInfo->item.iItem];

                    if (!context->UseKph && dispInfo->item.iSubItem == 0)
                    {
                        WCHAR value[PH_INT32_STR_LEN_1];

                        PhPrintUInt32(value, (ULONG)dispInfo->item.iItem + 1);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value, _TRUNCATE);
                    }
                    else if ((ULONG)dispInfo->item.iSubItem < RTL_NUMBER_OF(item->Text))
                    {
                        wcsncpy_s(
                            dispInfo->item.pszText,
                            dispInfo->item.cchTextMax,
                            PhGetStringOrEmpty(item->Text[dispInfo->item.iSubItem]),
                            _TRUNCATE
                            );
                    }
                }
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
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
