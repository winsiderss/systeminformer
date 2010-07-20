/*
 * Process Hacker - 
 *   logging system
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

#define LOG_PRIVATE
#include <phapp.h>
#include <settings.h>

PH_CIRCULAR_BUFFER_PVOID PhLogBuffer;
PH_CALLBACK PhLoggedCallback;

VOID PhLogInitialization()
{
    ULONG entries;

    entries = PhGetIntegerSetting(L"LogEntries");
    if (entries > 0x1000) entries = 0x1000;
    PhInitializeCircularBuffer_PVOID(&PhLogBuffer, entries);
    memset(PhLogBuffer.Data, 0, sizeof(PVOID) * PhLogBuffer.Size);

    PhInitializeCallback(&PhLoggedCallback);
}

PPH_LOG_ENTRY PhpCreateLogEntry(
    __in UCHAR Type
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhAllocate(sizeof(PH_LOG_ENTRY));
    memset(entry, 0, sizeof(PH_LOG_ENTRY));

    entry->Type = Type;
    PhQuerySystemTime(&entry->Time);

    return entry;
}

VOID PhpFreeLogEntry(
    __inout PPH_LOG_ENTRY Entry
    )
{
    if (Entry->Type >= PH_LOG_ENTRY_PROCESS_FIRST && Entry->Type <= PH_LOG_ENTRY_PROCESS_LAST)
    {
        PhDereferenceObject(Entry->Process.Name);
        if (Entry->Process.ParentName) PhDereferenceObject(Entry->Process.ParentName);
    }
    else if (Entry->Type >= PH_LOG_ENTRY_SERVICE_FIRST && Entry->Type <= PH_LOG_ENTRY_SERVICE_LAST)
    {
        PhDereferenceObject(Entry->Service.Name);
        PhDereferenceObject(Entry->Service.DisplayName);
    }
    else if (Entry->Type == PH_LOG_ENTRY_MESSAGE)
    {
        PhDereferenceObject(Entry->Message);
    }

    PhFree(Entry);
}

PPH_LOG_ENTRY PhpCreateProcessLogEntry(
    __in UCHAR Type,
    __in HANDLE ProcessId,
    __in PPH_STRING Name,
    __in_opt HANDLE ParentProcessId,
    __in_opt PPH_STRING ParentName
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type);
    entry->Process.ProcessId = ProcessId;
    PhReferenceObject(Name);
    entry->Process.Name = Name;

    entry->Process.ParentProcessId = ParentProcessId;

    if (ParentName)
    {
        PhReferenceObject(ParentName);
        entry->Process.ParentName = ParentName;
    }

    return entry;
}

PPH_LOG_ENTRY PhpCreateServiceLogEntry(
    __in UCHAR Type,
    __in PPH_STRING Name,
    __in PPH_STRING DisplayName
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type);
    PhReferenceObject(Name);
    entry->Service.Name = Name;
    PhReferenceObject(DisplayName);
    entry->Service.DisplayName = DisplayName;

    return entry;
}

PPH_LOG_ENTRY PhpCreateMessageLogEntry(
    __in UCHAR Type,
    __in PPH_STRING Message
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type);
    PhReferenceObject(Message);
    entry->Message = Message;

    return entry;
}

VOID PhpLogEntry(
    __in PPH_LOG_ENTRY Entry
    )
{
    PPH_LOG_ENTRY oldEntry;

    oldEntry = PhCircularBufferAdd2_PVOID(&PhLogBuffer, Entry);

    if (oldEntry)
        PhpFreeLogEntry(oldEntry);

    PhInvokeCallback(&PhLoggedCallback, Entry);
}

VOID PhClearLogEntries()
{
    ULONG i;

    for (i = 0; i < PhLogBuffer.Size; i++)
    {
        if (PhLogBuffer.Data[i])
            PhpFreeLogEntry(PhLogBuffer.Data[i]);
    }

    PhClearCircularBuffer_PVOID(&PhLogBuffer);
}

VOID PhLogProcessEntry(
    __in UCHAR Type,
    __in HANDLE ProcessId,
    __in PPH_STRING Name,
    __in_opt HANDLE ParentProcessId,
    __in_opt PPH_STRING ParentName
    )
{
    PhpLogEntry(PhpCreateProcessLogEntry(Type, ProcessId, Name, ParentProcessId, ParentName));
}

VOID PhLogServiceEntry(
    __in UCHAR Type,
    __in PPH_STRING Name,
    __in PPH_STRING DisplayName
    )
{
    PhpLogEntry(PhpCreateServiceLogEntry(Type, Name, DisplayName));
}

VOID PhLogMessageEntry(
    __in UCHAR Type,
    __in PPH_STRING Message
    )
{
    PhpLogEntry(PhpCreateMessageLogEntry(Type, Message));
}

PPH_STRING PhFormatLogEntry(
    __in PPH_LOG_ENTRY Entry
    )
{
    switch (Entry->Type)
    {
    case PH_LOG_ENTRY_PROCESS_CREATE:
        return PhFormatString(
            L"Process created: %s (%u) started by %s (%u)",
            Entry->Process.Name->Buffer,
            (ULONG)Entry->Process.ProcessId,
            PhGetStringOrDefault(Entry->Process.ParentName, L"Unknown Process"),
            (ULONG)Entry->Process.ParentProcessId
            );
    case PH_LOG_ENTRY_PROCESS_DELETE:
        return PhFormatString(L"Process terminated: %s (%u)", Entry->Process.Name->Buffer, (ULONG)Entry->Process.ProcessId);
    case PH_LOG_ENTRY_SERVICE_CREATE:
        return PhFormatString(L"Service created: %s (%s)", Entry->Service.Name->Buffer, Entry->Service.DisplayName->Buffer);
    case PH_LOG_ENTRY_SERVICE_DELETE:
        return PhFormatString(L"Service deleted: %s (%s)", Entry->Service.Name->Buffer, Entry->Service.DisplayName->Buffer);
    case PH_LOG_ENTRY_SERVICE_START:
        return PhFormatString(L"Service started: %s (%s)", Entry->Service.Name->Buffer, Entry->Service.DisplayName->Buffer);
    case PH_LOG_ENTRY_SERVICE_STOP:
        return PhFormatString(L"Service stopped: %s (%s)", Entry->Service.Name->Buffer, Entry->Service.DisplayName->Buffer);
    case PH_LOG_ENTRY_SERVICE_CONTINUE:
        return PhFormatString(L"Service continued: %s (%s)", Entry->Service.Name->Buffer, Entry->Service.DisplayName->Buffer);
    case PH_LOG_ENTRY_SERVICE_PAUSE:
        return PhFormatString(L"Service paused: %s (%s)", Entry->Service.Name->Buffer, Entry->Service.DisplayName->Buffer);
    case PH_LOG_ENTRY_MESSAGE:
        PhReferenceObject(Entry->Message);
        return Entry->Message;
    default:
        return PhCreateString(L"");
    }
}
