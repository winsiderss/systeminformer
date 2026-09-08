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

// A bounded ring of what happened: processes starting and exiting, services changing state, devices
// arriving and going away.
//
// System Informer logs all of this itself, but a plugin only receives the log entry as an opaque
// pointer plus a formatter that renders it as a sentence, and prose is exactly what this server does
// not return. The events are therefore built from the provider callbacks, which hand over the typed
// items: the same source the app's own log is written from.
//
// Nothing before the plugin loaded can be answered, and the ring is bounded, so this is a recent
// feed and not an audit log. Reads are by cursor rather than index because the ring moves under the
// reader: a client asks for what happened after the cursor it last saw and is told plainly how many
// events it missed, rather than being handed a shorter list that looks complete.

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

static CONST PCSTR AtEventKindNames[AtEventKindMaximum] =
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

// A process that has exited is gone from every list, so what it was has to be kept at the moment it
// went: the command line and image path are the point, since that is what an agent is asking about
// when it asks what just ran and died.

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

static AT_PROCESS_EXIT AtExitRing[AT_EXIT_RING_SIZE];
static ULONG AtExitNext = 0;
static ULONG AtExitCount = 0;

static PH_QUEUED_LOCK AtEventLock = PH_QUEUED_LOCK_INIT;
static AT_EVENT AtEventRing[AT_EVENT_RING_SIZE];
static ULONG64 AtEventNextCursor = 1;
static ULONG64 AtEventOldestCursor = 1;

static PH_CALLBACK_REGISTRATION AtEventProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION AtEventProcessRemovedRegistration;
static PH_CALLBACK_REGISTRATION AtEventServiceAddedRegistration;
static PH_CALLBACK_REGISTRATION AtEventServiceModifiedRegistration;
static PH_CALLBACK_REGISTRATION AtEventServiceRemovedRegistration;
static PH_CALLBACK_REGISTRATION AtEventDeviceRegistration;
static PH_CALLBACK_REGISTRATION AtEventProcessUpdatedRegistration;
static PH_CALLBACK_REGISTRATION AtEventServiceUpdatedRegistration;

// A provider raises an added event for everything that exists on its first run. That is the
// enumeration of what was already there, not news, and on this machine it is several hundred
// events that would fill the ring before anything real happened. Additions only count once the
// provider that raises them has completed a run.
static LONG AtEventProcessProviderRan = 0;
static LONG AtEventServiceProviderRan = 0;

// Lock held.
VOID AtpClearEvent(
    _Inout_ PAT_EVENT Event
    )
{
    PhClearReference(&Event->Name);
    PhClearReference(&Event->Detail);
    memset(Event, 0, sizeof(AT_EVENT));
}

// Takes the next slot, evicting the oldest when the ring is full. The returned entry is blank and
// stamped; the caller fills the rest under the same lock.
_Requires_lock_held_(AtEventLock)
PAT_EVENT AtpPushEvent(
    _In_ AT_EVENT_KIND Kind
    )
{
    PAT_EVENT event;

    event = &AtEventRing[AtEventNextCursor % AT_EVENT_RING_SIZE];

    if (event->Cursor != 0)
    {
        // Overwriting something a slow reader may not have seen: moving the oldest cursor is what
        // later tells that reader it missed events.
        AtpClearEvent(event);
        AtEventOldestCursor = AtEventNextCursor - AT_EVENT_RING_SIZE + 1;
    }

    event->Cursor = AtEventNextCursor++;
    event->Kind = Kind;
    PhQuerySystemTime(&event->Time);

    return event;
}

// Lock held.
VOID AtpRecordProcessExit(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN HaveExitStatus,
    _In_ NTSTATUS ExitStatus
    )
{
    PAT_PROCESS_EXIT exit;

    exit = &AtExitRing[AtExitNext];
    AtExitNext = (AtExitNext + 1) % AT_EXIT_RING_SIZE;

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
        AtExitCount++;
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

    if (!processItem || !ReadAcquire(&AtEventProcessProviderRan))
        return;

    PhAcquireQueuedLockExclusive(&AtEventLock);

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

    PhReleaseQueuedLockExclusive(&AtEventLock);
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

    PhAcquireQueuedLockExclusive(&AtEventLock);

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

    PhReleaseQueuedLockExclusive(&AtEventLock);
}

VOID AtpPushServiceEvent(
    _In_ AT_EVENT_KIND Kind,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PAT_EVENT event;

    PhAcquireQueuedLockExclusive(&AtEventLock);

    event = AtpPushEvent(Kind);
    PhSetReference(&event->Name, ServiceItem->Name);
    PhSetReference(&event->Detail, ServiceItem->DisplayName);

    if (ServiceItem->ProcessId)
        event->ProcessId = ServiceItem->ProcessId;

    PhReleaseQueuedLockExclusive(&AtEventLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventServiceAddedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter && ReadAcquire(&AtEventServiceProviderRan))
        AtpPushServiceEvent(AtEventServiceCreate, Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventProcessUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    WriteRelease(&AtEventProcessProviderRan, TRUE);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpEventServiceUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    WriteRelease(&AtEventServiceProviderRan, TRUE);
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

    // Only transitions. The provider raises this for any change to a service item, which on a
    // normal machine is hundreds of events a minute that say nothing an agent can act on and push
    // the process events out of the ring; "what changed about services" is what
    // list_services with since_snapshot_id answers.
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

    PhAcquireQueuedLockExclusive(&AtEventLock);

    event = AtpPushEvent(kind);
    PhMoveReference(&event->Name, name);

    PhReleaseQueuedLockExclusive(&AtEventLock);
}

VOID AtEventsInitialize(
    VOID
    )
{
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent),
        AtpEventProcessAddedCallback,
        NULL,
        &AtEventProcessAddedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent),
        AtpEventProcessRemovedCallback,
        NULL,
        &AtEventProcessRemovedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderAddedEvent),
        AtpEventServiceAddedCallback,
        NULL,
        &AtEventServiceAddedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent),
        AtpEventServiceModifiedCallback,
        NULL,
        &AtEventServiceModifiedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderRemovedEvent),
        AtpEventServiceRemovedCallback,
        NULL,
        &AtEventServiceRemovedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent),
        AtpEventDeviceCallback,
        NULL,
        &AtEventDeviceRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        AtpEventProcessUpdatedCallback,
        NULL,
        &AtEventProcessUpdatedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderUpdatedEvent),
        AtpEventServiceUpdatedCallback,
        NULL,
        &AtEventServiceUpdatedRegistration
        );
}

VOID AtEventsUninitialize(
    VOID
    )
{
    ULONG i;

    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent), &AtEventProcessAddedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent), &AtEventProcessRemovedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderAddedEvent), &AtEventServiceAddedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent), &AtEventServiceModifiedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderRemovedEvent), &AtEventServiceRemovedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent), &AtEventDeviceRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &AtEventProcessUpdatedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderUpdatedEvent), &AtEventServiceUpdatedRegistration);

    PhAcquireQueuedLockExclusive(&AtEventLock);

    for (i = 0; i < AT_EVENT_RING_SIZE; i++)
        AtpClearEvent(&AtEventRing[i]);

    for (i = 0; i < AT_EXIT_RING_SIZE; i++)
    {
        PhClearReference(&AtExitRing[i].Name);
        PhClearReference(&AtExitRing[i].FileName);
        PhClearReference(&AtExitRing[i].CommandLine);
        PhClearReference(&AtExitRing[i].UserName);
        PhClearReference(&AtExitRing[i].ParentName);
        memset(&AtExitRing[i], 0, sizeof(AT_PROCESS_EXIT));
    }

    AtExitNext = 0;
    AtExitCount = 0;

    PhReleaseQueuedLockExclusive(&AtEventLock);
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

    PhAcquireQueuedLockShared(&AtEventLock);

    // What was asked for but is no longer held. A client that has never read starts at the oldest
    // kept event and is not told it missed what happened before the plugin loaded, which it could
    // not have seen in any case.
    if (sinceCursor != 0 && sinceCursor + 1 < AtEventOldestCursor)
        dropped = AtEventOldestCursor - sinceCursor - 1;

    cursor = max(sinceCursor + 1, AtEventOldestCursor);
    nextCursor = sinceCursor;

    for (; cursor < AtEventNextCursor; cursor++)
    {
        PAT_EVENT event = &AtEventRing[cursor % AT_EVENT_RING_SIZE];
        PCSTR kind;
        PVOID row;

        if (event->Cursor != cursor)
            continue; // the slot moved on under a reader walking slowly

        kind = AtEventKindNames[event->Kind];

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

    if (cursor < AtEventNextCursor)
        hasMore = TRUE;

    PhReleaseQueuedLockShared(&AtEventLock);

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

    PhAcquireQueuedLockShared(&AtEventLock);

    // Newest first: what just died is what is being asked about.
    for (i = 0; i < AtExitCount; i++)
    {
        PAT_PROCESS_EXIT exit;
        PVOID row;

        index = (AtExitNext + AT_EXIT_RING_SIZE - 1 - i) % AT_EXIT_RING_SIZE;
        exit = &AtExitRing[index];

        if (!exit->Used)
            continue;

        if (!AtContainsString(exit->Name, nameContains) &&
            !AtContainsString(exit->CommandLine, nameContains))
        {
            continue;
        }

        if (havePid && exit->ProcessId != UlongToHandle((ULONG)pid))
            continue;

        // Non-zero, not NT_SUCCESS: a process exit code is not an NTSTATUS. A program that
        // returns 7 has failed, but 0x7 has severity zero and NT_SUCCESS calls it success. Zero is
        // the only value that means success in both readings.
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

    PhReleaseQueuedLockShared(&AtEventLock);

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
