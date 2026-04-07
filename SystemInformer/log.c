/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2021
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <settings.h>
#include <phsettings.h>

PH_CIRCULAR_BUFFER_PVOID PhLogBuffer;

VOID PhLogInitialization(
    VOID
    )
{
    ULONG entries;

    entries = PhGetIntegerSetting(SETTING_LOG_ENTRIES);
    if (entries > 0x1000) entries = 0x1000;
    PhInitializeCircularBuffer_PVOID(&PhLogBuffer, entries);
    memset(PhLogBuffer.Data, 0, sizeof(PVOID) * PhLogBuffer.Size);
}

PPH_LOG_ENTRY PhpCreateLogEntry(
    _In_ UCHAR Type
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
    _In_ _Post_invalid_ PPH_LOG_ENTRY Entry
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
    _In_ UCHAR Type,
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING Name,
    _In_opt_ HANDLE ParentProcessId,
    _In_opt_ PPH_STRING ParentName,
    _In_opt_ ULONG Status
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type);
    entry->Process.ProcessId = ProcessId;
    PhReferenceObject(Name);
    entry->Process.Name = Name;

    entry->Process.ParentProcessId = ParentProcessId;

    if (!PhIsNullOrEmptyString(ParentName))
    {
        PhReferenceObject(ParentName);
        entry->Process.ParentName = ParentName;
    }

    entry->Process.ExitStatus = Status;

    return entry;
}

PPH_LOG_ENTRY PhpCreateServiceLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING DisplayName
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

PPH_LOG_ENTRY PhpCreateDeviceLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Classification,
    _In_ PPH_STRING Name
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type);
    PhReferenceObject(Classification);
    entry->Device.Classification = Classification;
    PhReferenceObject(Name);
    entry->Device.Name = Name;

    return entry;
}

PPH_LOG_ENTRY PhpCreateMessageLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type);
    PhReferenceObject(Message);
    entry->Message = Message;

    return entry;
}

VOID PhpLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    PPH_LOG_ENTRY oldEntry;

    oldEntry = PhAddItemCircularBuffer2_PVOID(&PhLogBuffer, Entry);

    if (oldEntry)
        PhpFreeLogEntry(oldEntry);

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackLoggedEvent), Entry);
}

VOID PhClearLogEntries(
    VOID
    )
{
    ULONG i;

    for (i = 0; i < PhLogBuffer.Size; i++)
    {
        if (PhLogBuffer.Data[i])
            PhpFreeLogEntry(PhLogBuffer.Data[i]);
    }

    PhClearCircularBuffer_PVOID(&PhLogBuffer);
    memset(PhLogBuffer.Data, 0, sizeof(PVOID) * PhLogBuffer.Size);
}

VOID PhLogProcessEntry(
    _In_ UCHAR Type,
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING Name,
    _In_opt_ HANDLE ParentProcessId,
    _In_opt_ PPH_STRING ParentName,
    _In_opt_ ULONG Status
    )
{
    PhpLogEntry(PhpCreateProcessLogEntry(Type, ProcessId, Name, ParentProcessId, ParentName, Status));
}

VOID PhLogServiceEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING DisplayName
    )
{
    PhpLogEntry(PhpCreateServiceLogEntry(Type, Name, DisplayName));
}

VOID PhLogDeviceEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Classification,
    _In_ PPH_STRING Name
    )
{
    PhpLogEntry(PhpCreateDeviceLogEntry(Type, Classification, Name));
}

VOID PhLogMessageEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message
    )
{
    PhpLogEntry(PhpCreateMessageLogEntry(Type, Message));
}

PPH_STRING PhpFormatLogEntryToBuffer(
    _In_ PPH_FORMAT Format,
    _In_ ULONG Count
    )
{
    SIZE_T formatLength;
    WCHAR formatBuffer[0x80];

    if (PhFormatToBuffer(
        Format,
        Count,
        formatBuffer,
        sizeof(formatBuffer),
        &formatLength
        ))
    {
        PH_STRINGREF text;

        text.Length = formatLength - sizeof(UNICODE_NULL);
        text.Buffer = formatBuffer;

        return PhCreateString2(&text);
    }

    return PhFormat(Format, Count, 0x80);
}

PPH_STRING PhFormatLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    switch (Entry->Type)
    {
    case PH_LOG_ENTRY_PROCESS_CREATE:
        {
            PH_FORMAT format[8];

            // Process created: %s (%lu) started by %s (%lu)
            //PhInitFormatS(&format[0], L"Process created: ");
            PhInitFormatSR(&format[0], Entry->Process.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatU(&format[2], HandleToUlong(Entry->Process.ProcessId));
            PhInitFormatS(&format[3], L") started by ");
            if (Entry->Process.ParentName)
                PhInitFormatSR(&format[4], Entry->Process.ParentName->sr);
            else
                PhInitFormatS(&format[4], L"Unknown process");
            PhInitFormatS(&format[5], L" (");
            PhInitFormatU(&format[6], HandleToUlong(Entry->Process.ParentProcessId));
            PhInitFormatC(&format[7], L')');

            //return PhFormatString(
            //    L"Process created: %s (%lu) started by %s (%lu)",
            //    Entry->Process.Name->Buffer,
            //    HandleToUlong(Entry->Process.ProcessId),
            //    PhGetStringOrDefault(Entry->Process.ParentName, L"Unknown process"),
            //    HandleToUlong(Entry->Process.ParentProcessId)
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_PROCESS_DELETE:
        {
            PH_FORMAT format[5];

            // Process terminated: %s (%lu); exit status 0x%x
            //PhInitFormatS(&format[0], L"Process terminated: ");
            PhInitFormatSR(&format[0], Entry->Process.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatU(&format[2], HandleToUlong(Entry->Process.ProcessId));
            PhInitFormatS(&format[3], L"); exit status ");
            PhInitFormatX(&format[4], Entry->Process.ExitStatus);

            //return PhFormatString(
            //    L"Process terminated: %s (%lu); exit status 0x%x",
            //    Entry->Process.Name->Buffer,
            //    HandleToUlong(Entry->Process.ProcessId),
            //    Entry->Process.ExitStatus
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_CREATE:
        {
            PH_FORMAT format[4];

            // Service created: %s (%s)
            //PhInitFormatS(&format[0], L"Service created: ");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatC(&format[3], L')');

            //return PhFormatString(
            //    L"Service created: %s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_DELETE:
        {
            PH_FORMAT format[4];

            // Service deleted: %s (%s)
            //PhInitFormatS(&format[0], L"Service deleted: ");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatC(&format[3], L')');

            //return PhFormatString(
            //    L"Service deleted: %s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_START:
        {
            PH_FORMAT format[4];

            // Service started: %s (%s)
            //PhInitFormatS(&format[0], L"Service started: ");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatC(&format[3], L')');

            //return PhFormatString(
            //    L"Service started: %s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_STOP:
        {
            PH_FORMAT format[4];

            // Service stopped: %s (%s)
            //PhInitFormatS(&format[0], L"Service stopped: ");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatC(&format[3], L')');

            //return PhFormatString(
            //    L"Service stopped: %s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_CONTINUE:
        {
            PH_FORMAT format[4];

            // Service continued: %s (%s)
            //PhInitFormatS(&format[0], L"Service continued: ");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatC(&format[3], L')');

            //return PhFormatString(
            //    L"Service continued: %s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_PAUSE:
        {
            PH_FORMAT format[4];

            // Service paused: %s (%s)
            //PhInitFormatS(&format[0], L"Service paused: ");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatC(&format[3], L')');

            //return PhFormatString(
            //    L"Service paused: %s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_SERVICE_MODIFIED:
        {
            PH_FORMAT format[4];

            // Service modified: %s (%s)
            //PhInitFormatS(&format[0], L"Service modified: ");
            PhInitFormatSR(&format[0], Entry->Service.Name->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Service.DisplayName->sr);
            PhInitFormatC(&format[3], L')');

            //return PhFormatString(
            //    L"Service modified: %s (%s)",
            //    Entry->Service.Name->Buffer,
            //    Entry->Service.DisplayName->Buffer
            //    );
            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_DEVICE_REMOVED:
        {
            PH_FORMAT format[4];

            //PhInitFormatS(&format[0], L"Device removed: ");
            PhInitFormatSR(&format[0], Entry->Device.Classification->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Device.Name->sr);
            PhInitFormatC(&format[3], L')');

            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_DEVICE_ARRIVED:
        {
            PH_FORMAT format[4];

            //PhInitFormatS(&format[0], L"Device arrived: ");
            PhInitFormatSR(&format[0], Entry->Device.Classification->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatSR(&format[2], Entry->Device.Name->sr);
            PhInitFormatC(&format[3], L')');

            return PhpFormatLogEntryToBuffer(format, RTL_NUMBER_OF(format));
        }
    case PH_LOG_ENTRY_MESSAGE:
        PhReferenceObject(Entry->Message);
        return Entry->Message;
    default:
        return PhReferenceEmptyString();
    }
}

static CONST PH_KEY_VALUE_PAIR PhpLogEntryTypePairs[] =
{
    SIP(SREF(L"Unknown"), 0),
    SIP(SREF(L"Process created"), PH_LOG_ENTRY_PROCESS_CREATE),
    SIP(SREF(L"Process terminated"), PH_LOG_ENTRY_PROCESS_DELETE),
    SIP(SREF(L"Service created"), PH_LOG_ENTRY_SERVICE_CREATE),
    SIP(SREF(L"Service terminated"), PH_LOG_ENTRY_SERVICE_DELETE),
    SIP(SREF(L"Service started"), PH_LOG_ENTRY_SERVICE_START),
    SIP(SREF(L"Service terminated"), PH_LOG_ENTRY_SERVICE_STOP),
    SIP(SREF(L"Service continued"), PH_LOG_ENTRY_SERVICE_CONTINUE),
    SIP(SREF(L"Service paused"), PH_LOG_ENTRY_SERVICE_PAUSE),
    SIP(SREF(L"Service modified"), PH_LOG_ENTRY_SERVICE_MODIFIED),
    SIP(SREF(L"Device removed"), PH_LOG_ENTRY_DEVICE_REMOVED),
    SIP(SREF(L"Device arrived"), PH_LOG_ENTRY_DEVICE_ARRIVED)
};

PCPH_STRINGREF PhFormatLogType(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    PCPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        PhpLogEntryTypePairs,
        sizeof(PhpLogEntryTypePairs),
        Entry->Type,
        &string
        ))
    {
        return string;
    }

    return NULL;
}
