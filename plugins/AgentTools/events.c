/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

#define AT_EVENT_RING_SIZE 512

typedef enum _AT_EVENT_KIND
{
    AtEventProcessCreate,
    AtEventProcessExit,
    AtEventServiceCreate,
    AtEventServiceDelete,
    AtEventServiceStart,
    AtEventServiceStop,
    AtEventServiceContinue,
    AtEventServicePause,
    AtEventDeviceArrived,
    AtEventDeviceRemoved,
    AtEventKindMaximum
} AT_EVENT_KIND;

static CONST PCSTR AtpEventKindNames[AtEventKindMaximum] =
{
    "process_create",
    "process_exit",
    "service_create",
    "service_delete",
    "service_start",
    "service_stop",
    "service_continue",
    "service_pause",
    "device_arrived",
    "device_removed",
};

typedef struct _AT_EVENT
{
    ULONG64 Cursor;
    AT_EVENT_KIND Kind;
    LARGE_INTEGER Time;
    HANDLE ProcessId;
    ULONG64 SequenceNumber;
    HANDLE ParentProcessId;
    NTSTATUS ExitStatus;
    BOOLEAN HaveExitStatus;
    PPH_STRING Name;
    PPH_STRING Detail;
} AT_EVENT, *PAT_EVENT;

#define AT_EXIT_RING_SIZE 256

typedef struct _AT_PROCESS_EXIT
{
    BOOLEAN Used;
    HANDLE ProcessId;
    ULONG64 SequenceNumber;
    HANDLE ParentProcessId;
    ULONG SessionId;
    NTSTATUS ExitStatus;
    BOOLEAN HaveExitStatus;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    PPH_STRING Name;
    PPH_STRING FileName;
    PPH_STRING CommandLine;
    PPH_STRING UserName;
    PPH_STRING ParentName;
} AT_PROCESS_EXIT, *PAT_PROCESS_EXIT;

static AT_PROCESS_EXIT AtpExitRing[AT_EXIT_RING_SIZE];
static ULONG AtpExitNext = 0;
static ULONG AtpExitCount = 0;

static PH_QUEUED_LOCK AtpEventLock = PH_QUEUED_LOCK_INIT;
static AT_EVENT AtpEventRing[AT_EVENT_RING_SIZE];
static ULONG64 AtpEventNextCursor = 1;
static ULONG64 AtpEventOldestCursor = 1;

static PH_CALLBACK_REGISTRATION AtpEventProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION AtpEventProcessRemovedRegistration;
static PH_CALLBACK_REGISTRATION AtpEventServiceAddedRegistration;
static PH_CALLBACK_REGISTRATION AtpEventServiceModifiedRegistration;
static PH_CALLBACK_REGISTRATION AtpEventServiceRemovedRegistration;
static PH_CALLBACK_REGISTRATION AtpEventDeviceRegistration;
static PH_CALLBACK_REGISTRATION AtpEventProcessUpdatedRegistration;
static PH_CALLBACK_REGISTRATION AtpEventServiceUpdatedRegistration;

static LONG AtpEventProcessProviderRan = 0;
static LONG AtpEventServiceProviderRan = 0;

VOID AtpClearEvent(
    _Inout_ PAT_EVENT Event
    )
{
    PhClearReference(&Event->Name);
    PhClearReference(&Event->Detail);
    memset(Event, 0, sizeof(AT_EVENT));
}

_Requires_lock_held_(AtpEventLock)
PAT_EVENT AtpPushEvent(
    _In_ AT_EVENT_KIND Kind
    )
{
    PAT_EVENT event;

    event = &AtpEventRing[AtpEventNextCursor % AT_EVENT_RING_SIZE];

    if (event->Cursor != 0)
    {
        // Overwriting something a slow reader may not have seen: moving the oldest cursor is what
        // later tells that reader it missed events.
        AtpClearEvent(event);
        AtpEventOldestCursor = AtpEventNextCursor - AT_EVENT_RING_SIZE + 1;
    }

    event->Cursor = AtpEventNextCursor++;
    event->Kind = Kind;
    PhQuerySystemTime(&event->Time);

    return event;
}

VOID AtpRecordProcessExit(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN HaveExitStatus,
    _In_ NTSTATUS ExitStatus
    )
{
    PAT_PROCESS_EXIT exit;

    exit = &AtpExitRing[AtpExitNext];
    AtpExitNext = (AtpExitNext + 1) % AT_EXIT_RING_SIZE;

    if (exit->Used)
    {
        PhClearReference(&exit->Name);
        PhClearReference(&exit->FileName);
        PhClearReference(&exit->CommandLine);
        PhClearReference(&exit->UserName);
        PhClearReference(&exit->ParentName);
    }
    else
    {
        AtpExitCount++;
    }

    memset(exit, 0, sizeof(AT_PROCESS_EXIT));
    exit->Used = TRUE;
    exit->ProcessId = ProcessItem->ProcessId;
    exit->SequenceNumber = ProcessItem->ProcessSequenceNumber;
    exit->ParentProcessId = ProcessItem->ParentProcessId;
    exit->SessionId = ProcessItem->SessionId;
    exit->CreateTime = ProcessItem->CreateTime;
    exit->ExitStatus = ExitStatus;
    exit->HaveExitStatus = HaveExitStatus;
    PhQuerySystemTime(&exit->ExitTime);
    PhSetReference(&exit->Name, ProcessItem->ProcessName);
    PhSetReference(&exit->FileName, ProcessItem->FileName);
    PhSetReference(&exit->CommandLine, ProcessItem->CommandLine);
    PhSetReference(&exit->UserName, ProcessItem->UserName);

    if (ProcessItem->ParentProcessId)
    {
        PPH_PROCESS_ITEM parent;

        if (parent = PhReferenceProcessItem(ProcessItem->ParentProcessId))
        {
            PhSetReference(&exit->ParentName, parent->ProcessName);
            PhDereferenceObject(parent);
        }
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventProcessAddedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PAT_EVENT event;

    if (!processItem || !ReadAcquire(&AtpEventProcessProviderRan))
        return;

    PhAcquireQueuedLockExclusive(&AtpEventLock);

    event = AtpPushEvent(AtEventProcessCreate);
    event->ProcessId = processItem->ProcessId;
    event->SequenceNumber = processItem->ProcessSequenceNumber;
    event->ParentProcessId = processItem->ParentProcessId;
    PhSetReference(&event->Name, processItem->ProcessName);

    // The process the parent id refers to may already be gone; the name is what the cache has now.
    if (processItem->ParentProcessId)
    {
        PPH_PROCESS_ITEM parent;

        if (parent = PhReferenceProcessItem(processItem->ParentProcessId))
        {
            PhSetReference(&event->Detail, parent->ProcessName);
            PhDereferenceObject(parent);
        }
    }

    PhReleaseQueuedLockExclusive(&AtpEventLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventProcessRemovedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PAT_EVENT event;
    PROCESS_BASIC_INFORMATION basicInfo;
    BOOLEAN haveExitStatus = FALSE;

    if (!processItem)
        return;

    // The exit status is not kept anywhere: it is read from the handle the provider still holds
    // open at this moment, which is what the app's own log does from this same event.
    if (processItem->QueryHandle)
    {
        if (NT_SUCCESS(PhGetProcessBasicInformation(processItem->QueryHandle, &basicInfo)))
            haveExitStatus = TRUE;
    }

    PhAcquireQueuedLockExclusive(&AtpEventLock);

    event = AtpPushEvent(AtEventProcessExit);
    event->ProcessId = processItem->ProcessId;
    event->SequenceNumber = processItem->ProcessSequenceNumber;
    event->ParentProcessId = processItem->ParentProcessId;
    PhSetReference(&event->Name, processItem->ProcessName);

    if (haveExitStatus)
    {
        event->ExitStatus = basicInfo.ExitStatus;
        event->HaveExitStatus = TRUE;
    }

    AtpRecordProcessExit(processItem, haveExitStatus, haveExitStatus ? basicInfo.ExitStatus : 0);

    PhReleaseQueuedLockExclusive(&AtpEventLock);
}

VOID AtpPushServiceEvent(
    _In_ AT_EVENT_KIND Kind,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PAT_EVENT event;

    PhAcquireQueuedLockExclusive(&AtpEventLock);

    event = AtpPushEvent(Kind);
    PhSetReference(&event->Name, ServiceItem->Name);
    PhSetReference(&event->Detail, ServiceItem->DisplayName);

    if (ServiceItem->ProcessId)
        event->ProcessId = ServiceItem->ProcessId;

    PhReleaseQueuedLockExclusive(&AtpEventLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventServiceAddedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter && ReadAcquire(&AtpEventServiceProviderRan))
        AtpPushServiceEvent(AtEventServiceCreate, Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventProcessUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    WriteRelease(&AtpEventProcessProviderRan, TRUE);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventServiceUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    WriteRelease(&AtpEventServiceProviderRan, TRUE);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventServiceRemovedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter)
        AtpPushServiceEvent(AtEventServiceDelete, Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventServiceModifiedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA data = Parameter;
    AT_EVENT_KIND kind;

    if (!data || !data->ServiceItem)
        return;

    // Only transitions. The provider raises this for any change to a service item, which is
    // hundreds of events a minute that would push the process events out of the ring.
    if (data->OldService.State == data->ServiceItem->State)
        return;

    switch (data->ServiceItem->State)
    {
    case SERVICE_RUNNING:
        kind = data->OldService.State == SERVICE_PAUSED ? AtEventServiceContinue : AtEventServiceStart;
        break;
    case SERVICE_STOPPED:
        kind = AtEventServiceStop;
        break;
    case SERVICE_PAUSED:
        kind = AtEventServicePause;
        break;
    default:
        return; // a pending state on the way to one of the above
    }

    AtpPushServiceEvent(kind, data->ServiceItem);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventDeviceCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_DEVICE_NOTIFY notify = Parameter;
    PAT_EVENT event;
    AT_EVENT_KIND kind;
    PPH_STRING name = NULL;

    if (!notify)
        return;

    switch (notify->Action)
    {
    case PhDeviceNotifyInterfaceArrival:
        kind = AtEventDeviceArrived;
        name = PhFormatGuid(&notify->DeviceInterface.ClassGuid);
        break;
    case PhDeviceNotifyInterfaceRemoval:
        kind = AtEventDeviceRemoved;
        name = PhFormatGuid(&notify->DeviceInterface.ClassGuid);
        break;
    case PhDeviceNotifyInstanceStarted:
        kind = AtEventDeviceArrived;
        PhSetReference(&name, notify->DeviceInstance.InstanceId);
        break;
    case PhDeviceNotifyInstanceRemoved:
        kind = AtEventDeviceRemoved;
        PhSetReference(&name, notify->DeviceInstance.InstanceId);
        break;
    default:
        return; // enumeration, which is not an arrival
    }

    PhAcquireQueuedLockExclusive(&AtpEventLock);

    event = AtpPushEvent(kind);
    PhMoveReference(&event->Name, name);

    PhReleaseQueuedLockExclusive(&AtpEventLock);
}

VOID AtEventsInitialize(
    VOID
    )
{
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent),
        AtpEventProcessAddedCallback,
        NULL,
        &AtpEventProcessAddedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent),
        AtpEventProcessRemovedCallback,
        NULL,
        &AtpEventProcessRemovedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderAddedEvent),
        AtpEventServiceAddedCallback,
        NULL,
        &AtpEventServiceAddedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent),
        AtpEventServiceModifiedCallback,
        NULL,
        &AtpEventServiceModifiedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderRemovedEvent),
        AtpEventServiceRemovedCallback,
        NULL,
        &AtpEventServiceRemovedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent),
        AtpEventDeviceCallback,
        NULL,
        &AtpEventDeviceRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        AtpEventProcessUpdatedCallback,
        NULL,
        &AtpEventProcessUpdatedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderUpdatedEvent),
        AtpEventServiceUpdatedCallback,
        NULL,
        &AtpEventServiceUpdatedRegistration
        );
}

VOID AtEventsUninitialize(
    VOID
    )
{
    ULONG i;

    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent), &AtpEventProcessAddedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent), &AtpEventProcessRemovedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderAddedEvent), &AtpEventServiceAddedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent), &AtpEventServiceModifiedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderRemovedEvent), &AtpEventServiceRemovedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent), &AtpEventDeviceRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &AtpEventProcessUpdatedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderUpdatedEvent), &AtpEventServiceUpdatedRegistration);

    PhAcquireQueuedLockExclusive(&AtpEventLock);

    for (i = 0; i < AT_EVENT_RING_SIZE; i++)
        AtpClearEvent(&AtpEventRing[i]);

    for (i = 0; i < AT_EXIT_RING_SIZE; i++)
    {
        PhClearReference(&AtpExitRing[i].Name);
        PhClearReference(&AtpExitRing[i].FileName);
        PhClearReference(&AtpExitRing[i].CommandLine);
        PhClearReference(&AtpExitRing[i].UserName);
        PhClearReference(&AtpExitRing[i].ParentName);
        memset(&AtpExitRing[i], 0, sizeof(AT_PROCESS_EXIT));
    }

    AtpExitNext = 0;
    AtpExitCount = 0;

    PhReleaseQueuedLockExclusive(&AtpEventLock);
}

BOOLEAN AtpEventMatchesKinds(
    _In_opt_ PVOID Kinds,
    _In_ PCSTR Kind
    )
{
    ULONG count;
    ULONG i;

    if (!Kinds)
        return TRUE;

    count = PhGetJsonArrayLength(Kinds);

    for (i = 0; i < count; i++)
    {
        PPH_STRING value;
        PPH_BYTES utf8;
        BOOLEAN match;

        if (!(value = PhGetJsonObjectString(PhGetJsonArrayIndexObject(Kinds, i))))
            continue;

        utf8 = PhConvertUtf16ToUtf8Ex(value->Buffer, value->Length);
        match = utf8 && strcmp(utf8->Buffer, Kind) == 0;

        PhClearReference(&utf8);
        PhDereferenceObject(value);

        if (match)
            return TRUE;
    }

    return FALSE;
}

VOID AtpListRecentEvents(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PVOID structured;
    PVOID array;
    PVOID kinds;
    ULONG64 sinceCursor;
    ULONG64 pid;
    ULONG64 limit;
    ULONG64 nextCursor;
    ULONG64 dropped = 0;
    ULONG64 cursor;
    BOOLEAN havePid;
    BOOLEAN hasMore = FALSE;
    ULONG emitted = 0;

    if (!AtGetArgumentUInt64(Call->Arguments, "since_cursor", &sinceCursor))
        sinceCursor = 0;

    if (!AtGetArgumentUInt64(Call->Arguments, "limit", &limit) || limit == 0)
        limit = AT_ROWS_DEFAULT_LIMIT;

    limit = min(limit, AT_ROWS_MAXIMUM_LIMIT);
    havePid = AtGetArgumentUInt64(Call->Arguments, "pid", &pid) && pid <= MAXULONG;
    kinds = AtJsonGetObjectMember(Call->Arguments, "kinds", PH_JSON_OBJECT_TYPE_ARRAY);

    structured = PhCreateJsonObject();
    array = PhCreateJsonArray();

    PhAcquireQueuedLockShared(&AtpEventLock);

    // What was asked for but is no longer held. A client that has never read starts at the oldest
    // kept event and is not told it missed what happened before the plugin loaded.
    if (sinceCursor != 0 && sinceCursor + 1 < AtpEventOldestCursor)
        dropped = AtpEventOldestCursor - sinceCursor - 1;

    cursor = max(sinceCursor + 1, AtpEventOldestCursor);

    // Where the client has effectively got to, which is not always where it asked from: once the
    // ring has moved past its cursor, everything before the oldest event still held is gone. A
    // walked event advances this below, including one the filters reject, but a slot overwritten
    // under the walk does not - and a client that saw only those would be handed back its old
    // cursor and told about the same drop again on its next poll.
    nextCursor = cursor - 1;

    for (; cursor < AtpEventNextCursor; cursor++)
    {
        PAT_EVENT event = &AtpEventRing[cursor % AT_EVENT_RING_SIZE];
        PCSTR kind;
        PVOID row;

        if (event->Cursor != cursor)
            continue; // the slot moved on under a reader walking slowly

        kind = AtpEventKindNames[event->Kind];

        // A filtered-out event still advances the cursor: it has been seen and dealt with.
        if (!AtpEventMatchesKinds(kinds, kind) ||
            (havePid && event->ProcessId != UlongToHandle((ULONG)pid)))
        {
            nextCursor = cursor;
            continue;
        }

        if (emitted >= limit)
        {
            hasMore = TRUE;
            break;
        }

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "cursor", event->Cursor);
        PhAddJsonObject(row, "kind", kind);
        AtJsonAddTime(row, "time", &event->Time);

        if (event->ProcessId)
            PhAddJsonObjectUInt64(row, "pid", HandleToUlong(event->ProcessId));
        else
            AtJsonAddNull(row, "pid");

        if (event->SequenceNumber)
            PhAddJsonObjectUInt64(row, "process_sequence_number", event->SequenceNumber);
        else
            AtJsonAddNull(row, "process_sequence_number");

        AtJsonAddString(row, "name", event->Name);

        if (event->ParentProcessId)
            PhAddJsonObjectUInt64(row, "parent_pid", HandleToUlong(event->ParentProcessId));
        else
            AtJsonAddNull(row, "parent_pid");

        if (event->HaveExitStatus)
            AtJsonAddHex(row, "exit_status", (ULONG)event->ExitStatus);
        else
            AtJsonAddNull(row, "exit_status");

        AtJsonAddString(row, "detail", event->Detail);

        PhAddJsonArrayObject(array, row);
        nextCursor = cursor;
        emitted++;
    }

    if (cursor < AtpEventNextCursor)
        hasMore = TRUE;

    PhReleaseQueuedLockShared(&AtpEventLock);

    PhAddJsonObjectValue(structured, "events", array);
    PhAddJsonObjectUInt64(structured, "count", emitted);
    PhAddJsonObjectUInt64(structured, "next_cursor", nextCursor);
    PhAddJsonObjectUInt64(structured, "dropped", dropped);
    PhAddJsonObjectBoolean(structured, "has_more", hasMore);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtpListRecentProcessExits(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_ROWS rows;
    PVOID structured;
    PPH_STRING nameContains;
    ULONG64 pid;
    BOOLEAN havePid;
    BOOLEAN failedOnly;
    ULONG i;
    ULONG index;

    nameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    havePid = AtGetArgumentUInt64(Call->Arguments, "pid", &pid) && pid <= MAXULONG;
    failedOnly = AtJsonGetObjectBoolean(Call->Arguments, "failed_only");

    AtInitializeRows(&rows, Call->Arguments);

    PhAcquireQueuedLockShared(&AtpEventLock);

    // Newest first: what just died is what is being asked about.
    for (i = 0; i < AtpExitCount; i++)
    {
        PAT_PROCESS_EXIT exit;
        PVOID row;

        index = (AtpExitNext + AT_EXIT_RING_SIZE - 1 - i) % AT_EXIT_RING_SIZE;
        exit = &AtpExitRing[index];

        if (!exit->Used)
            continue;

        if (!AtContainsString(exit->Name, nameContains) &&
            !AtContainsString(exit->CommandLine, nameContains))
        {
            continue;
        }

        if (havePid && exit->ProcessId != UlongToHandle((ULONG)pid))
            continue;

        // Non-zero, not NT_SUCCESS: a process exit code is not an NTSTATUS. 0x7 has severity zero
        // and NT_SUCCESS calls it success, so zero is the only value that means success in both
        // readings.
        if (failedOnly && (!exit->HaveExitStatus || exit->ExitStatus == 0))
            continue;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "pid", HandleToUlong(exit->ProcessId));
        PhAddJsonObjectUInt64(row, "process_sequence_number", exit->SequenceNumber);
        AtJsonAddString(row, "name", exit->Name);
        AtJsonAddWin32FileName(row, "image_path", exit->FileName);
        AtJsonAddString(row, "command_line", exit->CommandLine);
        AtJsonAddString(row, "user", exit->UserName);
        PhAddJsonObjectUInt64(row, "session_id", exit->SessionId);
        AtJsonAddTime(row, "start_time", &exit->CreateTime);
        AtJsonAddTime(row, "exit_time", &exit->ExitTime);

        if (exit->CreateTime.QuadPart && exit->ExitTime.QuadPart > exit->CreateTime.QuadPart)
            AtJsonAddDuration(row, "lifetime_seconds", exit->ExitTime.QuadPart - exit->CreateTime.QuadPart);
        else
            AtJsonAddNull(row, "lifetime_seconds");

        if (exit->HaveExitStatus)
        {
            AtJsonAddHex(row, "exit_status", (ULONG)exit->ExitStatus);
            PhAddJsonObjectUInt64(row, "exit_code", (ULONG)exit->ExitStatus);
            PhAddJsonObjectBoolean(row, "exit_success", exit->ExitStatus == 0);
        }
        else
        {
            AtJsonAddNull(row, "exit_status");
            AtJsonAddNull(row, "exit_code");
            AtJsonAddNull(row, "exit_success");
        }

        if (exit->ParentProcessId)
            PhAddJsonObjectUInt64(row, "parent_pid", HandleToUlong(exit->ParentProcessId));
        else
            AtJsonAddNull(row, "parent_pid");

        AtJsonAddString(row, "parent_name", exit->ParentName);

        AtAddRow(&rows, row);
    }

    PhReleaseQueuedLockShared(&AtpEventLock);

    structured = PhCreateJsonObject();
    AtAddRows(structured, "processes", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&nameContains);
}

VOID AtEventInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListRecentEvents:
        AtpListRecentEvents(Call, Result);
        break;
    case AtActionListRecentProcessExits:
        AtpListRecentProcessExits(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
