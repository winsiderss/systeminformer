/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2026
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <settings.h>
#include <phsettings.h>
#include <procprv.h>

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
    _In_ UCHAR Type,
    _In_reads_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    PPH_LOG_ENTRY entry;
    SIZE_T entrySize;

    entrySize = FIELD_OFFSET(PH_LOG_ENTRY, Buffer) + BufferLength;
    entry = PhAllocate(entrySize);
    memset(entry, 0, entrySize);

    entry->Type = Type;
    PhQuerySystemTime(&entry->Time);
    entry->BufferLength = BufferLength;

    if (Buffer && BufferLength)
        memcpy(entry->Buffer, Buffer, BufferLength);

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
        if (Entry->Process.Record) PhDereferenceProcessRecord(Entry->Process.Record);
    }
    else if (Entry->Type >= PH_LOG_ENTRY_SERVICE_FIRST && Entry->Type <= PH_LOG_ENTRY_SERVICE_LAST)
    {
        PhDereferenceObject(Entry->Service.Name);
        PhDereferenceObject(Entry->Service.DisplayName);
        if (Entry->Service.FileName) PhDereferenceObject(Entry->Service.FileName);
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
    _In_opt_ ULONG Status,
    _In_opt_ PPH_PROCESS_RECORD Record
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, NULL, 0);
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

    if (Record)
    {
        PhReferenceProcessRecord(Record);
        entry->Process.Record = Record;
    }

    return entry;
}

PPH_LOG_ENTRY PhpCreateServiceLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING DisplayName,
    _In_opt_ PPH_STRING FileName
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, NULL, 0);
    PhReferenceObject(Name);
    entry->Service.Name = Name;
    PhReferenceObject(DisplayName);
    entry->Service.DisplayName = DisplayName;

    if (!PhIsNullOrEmptyString(FileName))
    {
        PhReferenceObject(FileName);
        entry->Service.FileName = FileName;
    }

    return entry;
}

PPH_LOG_ENTRY PhpCreateDeviceLogEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Classification,
    _In_ PPH_STRING Name
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, NULL, 0);
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

    entry = PhpCreateLogEntry(Type, NULL, 0);
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
    _In_opt_ ULONG Status,
    _In_opt_ PPH_PROCESS_RECORD Record
    )
{
    PhpLogEntry(PhpCreateProcessLogEntry(Type, ProcessId, Name, ParentProcessId, ParentName, Status, Record));
}

VOID PhLogServiceEntry(
    _In_ UCHAR Type,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING DisplayName,
    _In_opt_ PPH_STRING FileName
    )
{
    PhpLogEntry(PhpCreateServiceLogEntry(Type, Name, DisplayName, FileName));
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
    PhLogMessageEntryEx(Type, Message, NULL, 0);
}

VOID PhLogMessageEntryEx(
    _In_ UCHAR Type,
    _In_ PPH_STRING Message,
    _In_reads_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    PPH_LOG_ENTRY entry;

    entry = PhpCreateLogEntry(Type, Buffer, BufferLength);
    PhReferenceObject(Message);
    entry->Message = Message;

    PhpLogEntry(entry);
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

static PPH_STRING PhpFormatLogEntryExtra(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    if (Entry->BufferLength == 0)
        return PhReferenceEmptyString();

    return PhCreateStringEx((PVOID)Entry->Buffer, Entry->BufferLength);
}

static PPH_STRING PhpFormatServiceLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    PH_FORMAT format[16];
    ULONG count = 0;
    PPH_STRING version = NULL;
    PH_IMAGE_VERSION_INFO versionInfo = { 0 };

    if (PhCsEnableVersionSupport && !PhIsNullOrEmptyString(Entry->Service.FileName))
    {
        if (NT_SUCCESS(PhInitializeImageVersionInfoCached(&versionInfo, Entry->Service.FileName, FALSE, !!PhCsEnableVersionSupport)))
        {
            if (!PhIsNullOrEmptyString(versionInfo.FileVersion))
                version = versionInfo.FileVersion;
        }
    }

    PhInitFormatSR(&format[count++], Entry->Service.Name->sr);
    PhInitFormatS(&format[count++], L" (");
    PhInitFormatSR(&format[count++], Entry->Service.DisplayName->sr);
    PhInitFormatC(&format[count++], L')');

    if (!PhIsNullOrEmptyString(version))
    {
        PhInitFormatS(&format[count++], L" (v");
        PhInitFormatSR(&format[count++], version->sr);
        PhInitFormatC(&format[count++], L')');
    }

    PPH_STRING result = PhpFormatLogEntryToBuffer(format, count);
    PhDeleteImageVersionInfo(&versionInfo);
    return result;
}

PPH_STRING PhFormatLogEntry(
    _In_ PPH_LOG_ENTRY Entry
    )
{
    switch (Entry->Type)
    {
    case PH_LOG_ENTRY_PROCESS_CREATE:
        {
            PH_FORMAT format[20];
            ULONG count = 0;
            PPH_STRING processString = Entry->Process.Name;
            PPH_STRING version = NULL;
            PH_IMAGE_VERSION_INFO versionInfo = { 0 };

            if (Entry->Process.Record)
            {
                if (!PhIsNullOrEmptyString(Entry->Process.Record->CommandLine))
                    processString = Entry->Process.Record->CommandLine;

                if (!PhIsNullOrEmptyString(Entry->Process.Record->FileVersion))
                    version = Entry->Process.Record->FileVersion;
                else if (PhCsEnableVersionSupport && !PhIsNullOrEmptyString(Entry->Process.Record->FileName))
                {
                    if (NT_SUCCESS(PhInitializeImageVersionInfoCached(&versionInfo, Entry->Process.Record->FileName, TRUE, !!PhCsEnableVersionSupport)))
                    {
                        if (!PhIsNullOrEmptyString(versionInfo.FileVersion))
                            version = versionInfo.FileVersion;
                    }
                }
            }

            PhInitFormatSR(&format[count++], processString->sr);
            PhInitFormatS(&format[count++], L" (");
            PhInitFormatU(&format[count++], HandleToUlong(Entry->Process.ProcessId));
            PhInitFormatC(&format[count++], L')');

            if (!PhIsNullOrEmptyString(version))
            {
                PhInitFormatS(&format[count++], L" (v");
                PhInitFormatSR(&format[count++], version->sr);
                PhInitFormatC(&format[count++], L')');
            }

            PhInitFormatS(&format[count++], L" started by ");
            if (Entry->Process.ParentName)
                PhInitFormatSR(&format[count++], Entry->Process.ParentName->sr);
            else
                PhInitFormatS(&format[count++], L"Unknown process");
            PhInitFormatS(&format[count++], L" (");
            PhInitFormatU(&format[count++], HandleToUlong(Entry->Process.ParentProcessId));
            PhInitFormatC(&format[count++], L')');

            PPH_STRING result = PhpFormatLogEntryToBuffer(format, count);
            PhDeleteImageVersionInfo(&versionInfo);
            return result;
        }
    case PH_LOG_ENTRY_PROCESS_DELETE:
        {
            PH_FORMAT format[16];
            ULONG count = 0;
            PPH_STRING version = NULL;
            PH_IMAGE_VERSION_INFO versionInfo = { 0 };

            if (Entry->Process.Record)
            {
                if (!PhIsNullOrEmptyString(Entry->Process.Record->FileVersion))
                    version = Entry->Process.Record->FileVersion;
                else if (PhCsEnableVersionSupport && !PhIsNullOrEmptyString(Entry->Process.Record->FileName))
                {
                    if (NT_SUCCESS(PhInitializeImageVersionInfoCached(&versionInfo, Entry->Process.Record->FileName, TRUE, !!PhCsEnableVersionSupport)))
                    {
                        if (!PhIsNullOrEmptyString(versionInfo.FileVersion))
                            version = versionInfo.FileVersion;
                    }
                }
            }

            PhInitFormatSR(&format[count++], Entry->Process.Name->sr);
            PhInitFormatS(&format[count++], L" (");
            PhInitFormatU(&format[count++], HandleToUlong(Entry->Process.ProcessId));
            PhInitFormatC(&format[count++], L')');

            if (!PhIsNullOrEmptyString(version))
            {
                PhInitFormatS(&format[count++], L" (v");
                PhInitFormatSR(&format[count++], version->sr);
                PhInitFormatC(&format[count++], L')');
            }

            PhInitFormatS(&format[count++], L"; exit status ");
            PhInitFormatX(&format[count++], Entry->Process.ExitStatus);

            PPH_STRING result = PhpFormatLogEntryToBuffer(format, count);
            PhDeleteImageVersionInfo(&versionInfo);
            return result;
        }
    case PH_LOG_ENTRY_SERVICE_CREATE:
    case PH_LOG_ENTRY_SERVICE_DELETE:
    case PH_LOG_ENTRY_SERVICE_START:
    case PH_LOG_ENTRY_SERVICE_STOP:
    case PH_LOG_ENTRY_SERVICE_CONTINUE:
    case PH_LOG_ENTRY_SERVICE_PAUSE:
    case PH_LOG_ENTRY_SERVICE_MODIFIED:
        {
            return PhpFormatServiceLogEntry(Entry);
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
        {
            PPH_STRING extraString;

            PhReferenceObject(Entry->Message);

            if (Entry->BufferLength == 0)
                return Entry->Message;

            extraString = PH_AUTO_T(PH_STRING, PhpFormatLogEntryExtra(Entry));
            return PhaFormatString(L"%s [Extra: %s]", Entry->Message->Buffer, extraString->Buffer);
        }
    default:
        {
            PPH_STRING extraString;

            if (Entry->BufferLength == 0)
                return PhReferenceEmptyString();

            extraString = PH_AUTO_T(PH_STRING, PhpFormatLogEntryExtra(Entry));
            return PhaFormatString(L"[Extra: %s]", extraString->Buffer);
        }
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
