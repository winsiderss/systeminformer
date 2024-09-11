/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024
 *
 */
#include <phapp.h>
#include <kphcomms.h>

#include <informer.h>

#define KSI_MESSAGE_DRAIN_LIMIT   256
#define KSI_MESSAGE_DRAIN_TIMEOUT 5000000 // 500ms

typedef struct _KSI_MESSAGE_QUEUE_ENTRY
{
    SLIST_ENTRY Entry;
    PKPH_MESSAGE Message;
} KSI_MESSAGE_QUEUE_ENTRY, *PKSI_MESSAGE_QUEUE_ENTRY;

static PPH_OBJECT_TYPE KsiMessageObjectType = NULL;
static HANDLE KsiMessageWorkerThreadHandle = NULL;
static PH_RUNDOWN_PROTECT KsiMessageRundown;
static HANDLE KsiMessageQueueEvent = NULL;
static SLIST_HEADER KsiMessageQueueHeader;
static PH_FREE_LIST KsiMessageQueueFreeList;
static PPH_LIST KsiMessageOrderingList = NULL;
static LARGE_INTEGER KsiMessageLastDrain = { 0 };

PH_CALLBACK_DECLARE(PhInformerCallback);
PH_CALLBACK_DECLARE(PhInformerReplyCallback);

static int __cdecl PhpInformerMessageSort(
    _In_ void* Context,
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PKSI_MESSAGE_QUEUE_ENTRY entry1 = *(PKSI_MESSAGE_QUEUE_ENTRY*)elem1;
    PKSI_MESSAGE_QUEUE_ENTRY entry2 = *(PKSI_MESSAGE_QUEUE_ENTRY*)elem2;

    return int64cmp(entry1->Message->Header.TimeStamp.QuadPart, entry2->Message->Header.TimeStamp.QuadPart);
}

BOOLEAN PhpInformerDrainShouldDispatch(
    VOID
    )
{
    BOOLEAN result;
    LARGE_INTEGER systemTime;
    LARGE_INTEGER elapsed;

    PhQuerySystemTime(&systemTime);

    if (KsiMessageOrderingList->Count >= KSI_MESSAGE_DRAIN_LIMIT)
    {
        result = TRUE;
        goto CleanupExit;
    }

    elapsed.QuadPart = systemTime.QuadPart - KsiMessageLastDrain.QuadPart;
    if (elapsed.QuadPart >= KSI_MESSAGE_DRAIN_TIMEOUT)
    {
        result = TRUE;
        goto CleanupExit;
    }

    result = FALSE;

CleanupExit:

    if (result)
        KsiMessageLastDrain = systemTime;

    return result;
}

BOOLEAN PhpInformerDrain(
    _In_ BOOLEAN Force
    )
{
    PSLIST_ENTRY entry;
    PKSI_MESSAGE_QUEUE_ENTRY queueEntry;

    entry = InterlockedFlushSList(&KsiMessageQueueHeader);

    if (!entry)
    {
        if (Force)
            goto ForceDrain;

        return TRUE;
    }

    while (entry)
    {
        queueEntry = CONTAINING_RECORD(entry, KSI_MESSAGE_QUEUE_ENTRY, Entry);
        PhAddItemList(KsiMessageOrderingList, queueEntry);
        entry = entry->Next;
    }

    if (!Force && !PhpInformerDrainShouldDispatch())
        return FALSE;

ForceDrain:

    qsort_s(
        KsiMessageOrderingList->Items,
        KsiMessageOrderingList->Count,
        sizeof(PVOID),
        PhpInformerMessageSort,
        NULL
        );

    for (ULONG i = 0; i < KsiMessageOrderingList->Count; i++)
    {
        queueEntry = KsiMessageOrderingList->Items[i];

        PhInvokeCallback(&PhInformerCallback, queueEntry->Message);
        PhDereferenceObject(queueEntry->Message);
        PhFreeToFreeList(&KsiMessageQueueFreeList, queueEntry);
    }

    PhClearList(KsiMessageOrderingList);

    return FALSE;
}

NTSTATUS NTAPI PhpInformerMessageWorker(
    _In_ PVOID Context
    )
{
    BOOLEAN exit = FALSE;

    PhSetThreadName(NtCurrentThread(), L"InformerMessageWorker");

    for (NOTHING; !exit; NtWaitForSingleObject(KsiMessageQueueEvent, FALSE, NULL))
    {
        for (BOOLEAN drained = FALSE; !drained; NOTHING)
        {
            if (!PhAcquireRundownProtection(&KsiMessageRundown))
            {
                exit = TRUE;
                break;
            }

            if (PhpInformerDrain(FALSE))
            {
                drained = TRUE;
                NtResetEvent(KsiMessageQueueEvent, NULL);
            }

            PhReleaseRundownProtection(&KsiMessageRundown);
        }

        PhpInformerDrain(TRUE);
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhInformerReply(
    _Inout_ PPH_INFORMER_REPLY_CONTEXT Context,
    _In_ PKPH_MESSAGE ReplyMessage
    )
{
    NTSTATUS status;

    if (Context->Handled)
        return STATUS_INVALID_PARAMETER_1;

    if (NT_SUCCESS(status = KphCommsReplyMessage(Context->ReplyToken, ReplyMessage)))
        Context->Handled = TRUE;

    return status;
}

BOOLEAN PhInformerDispatch(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    )
{
    PKPH_MESSAGE message;
    PH_INFORMER_REPLY_CONTEXT context;

    if (!PhAcquireRundownProtection(&KsiMessageRundown))
        return FALSE;

    message = PhCreateObject(Message->Header.Size, KsiMessageObjectType);
    memcpy(message, Message, Message->Header.Size);

    context.Message = message;
    context.ReplyToken = ReplyToken;
    context.Handled = FALSE;

    PhInvokeCallback(&PhInformerReplyCallback, &context);

    if (KsiMessageWorkerThreadHandle)
    {
        PKSI_MESSAGE_QUEUE_ENTRY entry;

        entry = PhAllocateFromFreeList(&KsiMessageQueueFreeList);
        entry->Message = message;
        if (!InterlockedPushEntrySList(&KsiMessageQueueHeader, &entry->Entry))
            NtSetEvent(KsiMessageQueueEvent, NULL);
    }
    else
    {
        PhDereferenceObject(message);
    }

    PhReleaseRundownProtection(&KsiMessageRundown);

    return context.Handled;
}

VOID PhInformerInitialize(
    VOID
    )
{
    PhInitializeRundownProtection(&KsiMessageRundown);
    KsiMessageObjectType = PhCreateObjectType(L"KsiMessage", 0, NULL);
    KsiMessageOrderingList = PhCreateList(KSI_MESSAGE_DRAIN_LIMIT);
    PhInitializeFreeList(&KsiMessageQueueFreeList, sizeof(KSI_MESSAGE_QUEUE_ENTRY), KSI_MESSAGE_DRAIN_LIMIT);
    InitializeSListHead(&KsiMessageQueueHeader);
    if (NT_SUCCESS(NtCreateEvent(&KsiMessageQueueEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE)))
    {
        KsiMessageWorkerThreadHandle = PhCreateThread(0, PhpInformerMessageWorker, NULL);
    }
}

VOID PhInformerStop(
    VOID
    )
{
    PhWaitForRundownProtection(&KsiMessageRundown);
    if (KsiMessageQueueEvent)
    {
        NtSetEvent(KsiMessageQueueEvent, NULL);
        if (KsiMessageWorkerThreadHandle)
        {
            NtWaitForSingleObject(KsiMessageWorkerThreadHandle, FALSE, NULL);
            NtClose(KsiMessageWorkerThreadHandle);
            KsiMessageWorkerThreadHandle = NULL;
        }
        NtClose(KsiMessageQueueEvent);
        KsiMessageQueueEvent = NULL;
    }
    PhDeleteFreeList(&KsiMessageQueueFreeList);
    PhDereferenceObject(KsiMessageOrderingList);
}
