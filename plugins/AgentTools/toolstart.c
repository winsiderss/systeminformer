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

typedef struct _AT_STARTUP_CONTEXT
{
    PVOID Entries;
    PPH_STRING NameContains;
    PCSTR Location;
    PCSTR Scope;
    PCSTR Kind;
    ULONG Count;
} AT_STARTUP_CONTEXT, *PAT_STARTUP_CONTEXT;

static BOOLEAN AtpStartupMatches(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_opt_ PPH_STRING Name,
    _In_opt_ PPH_STRING Command
    )
{
    if (!Context->NameContains)
        return TRUE;

    return AtContainsString(Name, Context->NameContains) || AtContainsString(Command, Context->NameContains);
}

static VOID AtpAddStartupEntry(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_opt_ PPH_STRING Name,
    _In_opt_ PPH_STRING Command
    )
{
    PVOID entry;

    if (!AtpStartupMatches(Context, Name, Command))
        return;

    entry = PhCreateJsonObject();
    AtJsonAddString(entry, "name", Name);
    AtJsonAddString(entry, "command", Command);
    PhAddJsonObject(entry, "location", Context->Location);
    PhAddJsonObject(entry, "scope", Context->Scope);
    PhAddJsonObject(entry, "kind", Context->Kind);
    PhAddJsonArrayObject(Context->Entries, entry);
    Context->Count++;
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
static BOOLEAN NTAPI AtpStartupValueCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PAT_STARTUP_CONTEXT context = Context;
    PPH_STRING name;
    PPH_STRING command = NULL;

    if (Information->Type == REG_SZ || Information->Type == REG_EXPAND_SZ)
    {
        SIZE_T dataLength = Information->DataLength;

        // Trim a trailing null from the value if present.
        if (dataLength >= sizeof(WCHAR) &&
            *(PWCHAR)PTR_ADD_OFFSET(Information, Information->DataOffset + dataLength - sizeof(WCHAR)) == UNICODE_NULL)
        {
            dataLength -= sizeof(WCHAR);
        }

        command = PhCreateStringEx(PTR_ADD_OFFSET(Information, Information->DataOffset), dataLength);
    }

    name = PhCreateStringEx(Information->Name, Information->NameLength);
    AtpAddStartupEntry(context, name, command);

    PhClearReference(&name);
    PhClearReference(&command);

    return TRUE;
}

static VOID AtpReadRunKey(
    _In_ HANDLE RootDirectory,
    _In_ PCWSTR SubKey,
    _In_ PCSTR Location,
    _In_ PCSTR Scope,
    _In_ PCSTR Kind,
    _In_ PPH_STRING NameContains,
    _In_ PVOID Entries,
    _Inout_ PULONG Count
    )
{
    AT_STARTUP_CONTEXT context;
    HANDLE keyHandle;
    PH_STRINGREF subKey;

    context.Entries = Entries;
    context.NameContains = NameContains;
    context.Location = Location;
    context.Scope = Scope;
    context.Kind = Kind;
    context.Count = 0;

    PhInitializeStringRef(&subKey, SubKey);

    if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_QUERY_VALUE, RootDirectory, &subKey, 0)))
    {
        PhEnumerateValueKey(keyHandle, KeyValueFullInformation, AtpStartupValueCallback, &context);
        NtClose(keyHandle);
    }

    *Count += context.Count;
}

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN NTAPI AtpStartupFolderCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PAT_STARTUP_CONTEXT context = Context;
    PPH_STRING name;

    if (Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return TRUE;

    name = PhCreateStringEx(Information->FileName, Information->FileNameLength);

    // Skip the "." and ".." entries and desktop.ini.
    if (!PhEqualStringZ(name->Buffer, L".", TRUE) &&
        !PhEqualStringZ(name->Buffer, L"..", TRUE) &&
        !PhEqualStringZ(name->Buffer, L"desktop.ini", TRUE))
    {
        AtpAddStartupEntry(context, name, NULL);
    }

    PhDereferenceObject(name);

    return TRUE;
}

static VOID AtpReadStartupFolder(
    _In_ ULONG Folder,
    _In_ PCSTR Scope,
    _In_ PPH_STRING NameContains,
    _In_ PVOID Entries,
    _Inout_ PULONG Count
    )
{
    static PH_STRINGREF startupSuffix = PH_STRINGREF_INIT(L"\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
    AT_STARTUP_CONTEXT context;
    PPH_STRING folderPath;
    PPH_BYTES locationUtf8;
    HANDLE directoryHandle;

    if (!(folderPath = PhGetKnownLocation(Folder, &startupSuffix, FALSE)))
        return;

    context.Entries = Entries;
    context.NameContains = NameContains;
    context.Scope = Scope;
    context.Kind = "startup_folder";
    context.Count = 0;

    // The location string is the folder path itself.
    if (locationUtf8 = PhConvertUtf16ToUtf8Ex(folderPath->Buffer, folderPath->Length))
    {
        context.Location = locationUtf8->Buffer;

        if (NT_SUCCESS(PhCreateFileWin32(
            &directoryHandle,
            PhGetString(folderPath),
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            FILE_ATTRIBUTE_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            PhEnumDirectoryFile(directoryHandle, NULL, AtpStartupFolderCallback, &context);
            NtClose(directoryHandle);
        }

        PhDereferenceObject(locationUtf8);
    }

    PhDereferenceObject(folderPath);

    *Count += context.Count;
}

VOID AtListStartupEntries(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING nameContains;
    PVOID structured;
    PVOID entries;
    ULONG count = 0;

    nameContains = AtGetArgumentString(Call->Arguments, "name_contains");

    structured = PhCreateJsonObject();
    entries = PhCreateJsonArray();

    AtpReadRunKey(PH_KEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", "machine", "registry_run", nameContains, entries, &count);
    AtpReadRunKey(PH_KEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
        "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", "machine", "registry_run_once", nameContains, entries, &count);
    AtpReadRunKey(PH_KEY_LOCAL_MACHINE, L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run",
        "HKLM\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", "machine", "registry_run", nameContains, entries, &count);
    AtpReadRunKey(PH_KEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", "user", "registry_run", nameContains, entries, &count);
    AtpReadRunKey(PH_KEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
        "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", "user", "registry_run_once", nameContains, entries, &count);

    AtpReadStartupFolder(PH_FOLDERID_RoamingAppData, "user", nameContains, entries, &count);
    AtpReadStartupFolder(PH_FOLDERID_ProgramData, "machine", nameContains, entries, &count);

    PhAddJsonObjectValue(structured, "entries", entries);
    PhAddJsonObjectUInt64(structured, "count", count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&nameContains);
}
