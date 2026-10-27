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

typedef enum _AT_STARTUP_IMAGE
{
    AtStartupImageNone,
    AtStartupImageCommandLine,
    AtStartupImageModule,
    AtStartupImagePath
} AT_STARTUP_IMAGE;

typedef struct _AT_STARTUP_CONTEXT
{
    PAT_ROWS Entries;
    PPH_STRING NameContains;
    PPH_STRING KindFilter;
    BOOLEAN Verify;
    ULONG UnreadableCount;

    PPH_STRING Location;
    PCWSTR Scope;
    PCWSTR Kind;
    AT_STARTUP_IMAGE ImageKind;
    PCWSTR SubKeyValueName;
    BOOLEAN ClsidInData;
} AT_STARTUP_CONTEXT, *PAT_STARTUP_CONTEXT;

typedef struct _AT_STARTUP_KEY
{
    BOOLEAN Machine;
    PCWSTR SubKey;
    PCWSTR Location;
} AT_STARTUP_KEY, *PAT_STARTUP_KEY;

typedef struct _AT_STARTUP_VALUE
{
    PCWSTR Name;
    AT_STARTUP_IMAGE ImageKind;
    BOOLEAN Separated;
} AT_STARTUP_VALUE, *PAT_STARTUP_VALUE;

FORCEINLINE HANDLE AtpStartupRoot(
    _In_ BOOLEAN Machine
    )
{
    return Machine ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER;
}

FORCEINLINE VOID AtpStartupUnreadable(
    _Inout_ PAT_STARTUP_CONTEXT Context,
    _In_ NTSTATUS Status
    )
{
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND)
        Context->UnreadableCount++;
}

FORCEINLINE PCWSTR AtpStartupScope(
    _In_ BOOLEAN Machine
    )
{
    return Machine ? L"machine" : L"user";
}

BOOLEAN AtpSetStartupSource(
    _Inout_ PAT_STARTUP_CONTEXT Context,
    _In_ PCWSTR Location,
    _In_ PCWSTR Scope,
    _In_ PCWSTR Kind,
    _In_ AT_STARTUP_IMAGE ImageKind
    )
{
    if (Context->KindFilter && !PhEqualString2(Context->KindFilter, Kind, TRUE))
        return FALSE;

    PhMoveReference(&Context->Location, PhCreateString(Location));
    Context->Scope = Scope;
    Context->Kind = Kind;
    Context->ImageKind = ImageKind;
    Context->SubKeyValueName = NULL;
    Context->ClsidInData = FALSE;

    return TRUE;
}

PPH_STRING AtpResolveModuleName(
    _In_ PPH_STRING Name,
    _Out_ PBOOLEAN Exists
    )
{
    static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\\");
    static CONST PH_STRINGREF extension = PH_STRINGREF_INIT(L".dll");
    PPH_STRING systemDirectory;
    PPH_STRING fileName;
    PPH_STRING path;

    *Exists = FALSE;

    // A name with a path in it is what it says it is.
    if (PhFindCharInString(Name, 0, L'\\') != SIZE_MAX || PhFindCharInString(Name, 0, L':') != SIZE_MAX)
    {
        *Exists = PhDoesFileExistWin32(PhGetString(Name));
        return PhReferenceObject(Name);
    }

    // The loader appends .dll to a package name with no extension; "msv1_0" is a module, not an
    // extensionless file.
    if (PhFindCharInString(Name, 0, L'.') != SIZE_MAX)
        fileName = PhReferenceObject(Name);
    else
        fileName = PhConcatStringRef2(&Name->sr, &extension);

    if (systemDirectory = PhGetSystemDirectory())
    {
        path = PhConcatStringRef3(&systemDirectory->sr, &separator, &fileName->sr);
        PhDereferenceObject(systemDirectory);

        if (PhDoesFileExistWin32(PhGetString(path)))
        {
            PhDereferenceObject(fileName);
            *Exists = TRUE;
            return path;
        }

        PhDereferenceObject(path);
    }

    // Not where it would be loaded from: the name as written comes back rather than a path that was
    // never there.
    return fileName;
}

BOOLEAN AtpResolveStartupImage(
    _In_opt_ PPH_STRING Image,
    _In_ AT_STARTUP_IMAGE Kind,
    _Out_ PPH_STRING* ImagePath,
    _Out_ PBOOLEAN Exists
    )
{
    PPH_STRING expanded;
    PPH_STRING path = NULL;
    BOOLEAN exists = FALSE;

    *ImagePath = NULL;
    *Exists = FALSE;

    if (Kind == AtStartupImageNone || PhIsNullOrEmptyString(Image))
        return FALSE;

    if (!(expanded = PhExpandEnvironmentStrings(&Image->sr)))
        expanded = PhReferenceObject(Image);

    switch (Kind)
    {
    case AtStartupImageCommandLine:
        {
            PH_STRINGREF fileName;
            PH_STRINGREF arguments;
            PPH_STRING fullFileName;

            // The fuzzy parse is how the application tells a program from its arguments when
            // neither is quoted; it hands back a full path only when it found one.
            if (PhParseCommandLineFuzzy(&expanded->sr, &fileName, &arguments, &fullFileName))
            {
                if (fullFileName)
                {
                    path = fullFileName;
                    exists = TRUE;
                }
                else if (fileName.Length)
                {
                    path = PhCreateString2(&fileName);
                    exists = PhDoesFileExistWin32(PhGetString(path));
                }
            }
            else
            {
                PH_STRINGREF rest;

                // Nothing it tried was on disk, so it handed back the whole line. The first token
                // is the program, which is what matters for an entry pointing at a file that is not
                // there.
                if (!PhSplitStringRefAtChar(&expanded->sr, L' ', &fileName, &rest))
                    fileName = expanded->sr;

                if (fileName.Length)
                {
                    path = PhCreateString2(&fileName);
                    exists = PhDoesFileExistWin32(PhGetString(path));
                }
            }
        }
        break;
    case AtStartupImageModule:
        path = AtpResolveModuleName(expanded, &exists);
        break;
    case AtStartupImagePath:
        path = PhReferenceObject(expanded);
        exists = PhDoesFileExistWin32(PhGetString(path));
        break;
    case AtStartupImageNone:
        break;
    }

    PhDereferenceObject(expanded);

    *ImagePath = path;
    *Exists = exists;

    return !!path;
}

BOOLEAN AtpStartupMatches(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_opt_ PPH_STRING Name,
    _In_opt_ PPH_STRING Command
    )
{
    if (Context->KindFilter && !PhEqualString2(Context->KindFilter, Context->Kind, TRUE))
        return FALSE;

    if (!Context->NameContains)
        return TRUE;

    return AtContainsString(Name, Context->NameContains) || AtContainsString(Command, Context->NameContains);
}

VOID AtpAddStartupEntry(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_opt_ PPH_STRING Name,
    _In_opt_ PPH_STRING Command,
    _In_opt_ PPH_STRING Image,
    _In_opt_ PBOOLEAN Enabled
    )
{
    PVOID entry;
    PPH_STRING imagePath = NULL;
    PPH_STRING signer = NULL;
    VERIFY_RESULT verifyResult = VrUnknown;
    BOOLEAN imageExists = FALSE;
    BOOLEAN resolved;

    if (!AtpStartupMatches(Context, Name, Command))
        return;

    resolved = AtpResolveStartupImage(Image ? Image : Command, Context->ImageKind, &imagePath, &imageExists);

    // Only a file that is there is verified; a missing one answers unknown.
    if (Context->Verify && imageExists)
        verifyResult = AtVerifyFileName(imagePath, &signer);

    entry = PhCreateJsonObject();
    AtJsonAddString(entry, "name", Name);
    AtJsonAddString(entry, "command", Command);
    AtJsonAddString(entry, "location", Context->Location);
    AtJsonAddStringZ(entry, "scope", Context->Scope);
    AtJsonAddStringZ(entry, "kind", Context->Kind);
    AtJsonAddString(entry, "image_path", imagePath);

    if (resolved)
        PhAddJsonObjectBoolean(entry, "image_exists", imageExists);
    else
        AtJsonAddNull(entry, "image_exists");

    AtJsonAddStringZ(entry, "verify_result", AtVerifyResultString(verifyResult));
    AtJsonAddString(entry, "verify_signer", signer);

    if (Enabled)
        PhAddJsonObjectBoolean(entry, "enabled", *Enabled);
    else
        AtJsonAddNull(entry, "enabled");

    AtAddRow(Context->Entries, entry);

    PhClearReference(&signer);
    PhClearReference(&imagePath);
}

VOID AtpAddSeparatedEntries(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_opt_ PPH_STRING Name,
    _In_ PPH_STRING Value,
    _In_ PCPH_STRINGREF Separators,
    _In_opt_ PBOOLEAN Enabled
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    remaining = Value->sr;

    while (remaining.Length)
    {
        PPH_STRING element;

        PhSplitStringRefEx(&remaining, Separators, PH_SPLIT_AT_CHAR_SET, &part, &remaining, NULL);
        PhTrimStringRef(&part, &whitespace, 0);

        if (!part.Length)
            continue;

        element = PhCreateString2(&part);
        AtpAddStartupEntry(Context, Name, element, NULL, Enabled);
        PhDereferenceObject(element);
    }
}

PPH_STRING AtpGetRegistryStringData(
    _In_ PKEY_VALUE_FULL_INFORMATION Information
    )
{
    SIZE_T dataLength;

    if (Information->Type != REG_SZ && Information->Type != REG_EXPAND_SZ)
        return NULL;

    // DataLength is not guaranteed to be a whole number of WCHARs; drop a dangling odd byte.
    dataLength = Information->DataLength & ~(sizeof(WCHAR) - 1);

    if (dataLength >= sizeof(WCHAR) &&
        *(PWCHAR)PTR_ADD_OFFSET(Information, Information->DataOffset + dataLength - sizeof(WCHAR)) == UNICODE_NULL)
    {
        dataLength -= sizeof(WCHAR);
    }

    return PhCreateStringEx(PTR_ADD_OFFSET(Information, Information->DataOffset), dataLength);
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpStartupValueCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PAT_STARTUP_CONTEXT context = Context;
    PKEY_VALUE_FULL_INFORMATION information = Information;
    PPH_STRING name;
    PPH_STRING command;

    if (!context)
        return FALSE;

    command = AtpGetRegistryStringData(information);
    name = PhCreateStringEx(information->Name, information->NameLength);

    AtpAddStartupEntry(context, name, command, NULL, NULL);

    PhClearReference(&name);
    PhClearReference(&command);

    return TRUE;
}

VOID AtpEnumerateValueKey(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_ BOOLEAN Machine,
    _In_ PCWSTR SubKey,
    _In_ PPH_ENUM_KEY_CALLBACK Callback
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PH_STRINGREF subKey;

    PhInitializeStringRef(&subKey, SubKey);

    status = PhOpenKey(&keyHandle, KEY_QUERY_VALUE, AtpStartupRoot(Machine), &subKey, 0);

    if (NT_SUCCESS(status))
    {
        status = PhEnumerateValueKey(keyHandle, KeyValueFullInformation, Callback, Context);
        NtClose(keyHandle);
    }

    AtpStartupUnreadable(Context, status);
}

VOID AtpReadRunKeys(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    static CONST AT_STARTUP_KEY runKeys[] =
    {
        { TRUE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
        { TRUE, L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run",
            L"HKLM\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run" },
        { FALSE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
    };
    static CONST AT_STARTUP_KEY runOnceKeys[] =
    {
        { TRUE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
            L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
        { FALSE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
            L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
    };
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(runKeys); i++)
    {
        if (!AtpSetStartupSource(
            Context,
            runKeys[i].Location,
            AtpStartupScope(runKeys[i].Machine),
            L"registry_run",
            AtStartupImageCommandLine
            ))
        {
            break;
        }

        AtpEnumerateValueKey(Context, runKeys[i].Machine, runKeys[i].SubKey, AtpStartupValueCallback);
    }

    for (i = 0; i < RTL_NUMBER_OF(runOnceKeys); i++)
    {
        if (!AtpSetStartupSource(
            Context,
            runOnceKeys[i].Location,
            AtpStartupScope(runOnceKeys[i].Machine),
            L"registry_run_once",
            AtStartupImageCommandLine
            ))
        {
            break;
        }

        AtpEnumerateValueKey(Context, runOnceKeys[i].Machine, runOnceKeys[i].SubKey, AtpStartupValueCallback);
    }
}

_Function_class_(PH_ENUM_DIRECTORY_FILE)
BOOLEAN NTAPI AtpStartupFolderCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\\");
    PAT_STARTUP_CONTEXT context = Context;
    PPH_STRING name;

    if (!context)
        return FALSE;

    if (Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return TRUE;

    name = PhCreateStringEx(Information->FileName, Information->FileNameLength);

    if (!PhEqualStringZ(name->Buffer, L".", TRUE) &&
        !PhEqualStringZ(name->Buffer, L"..", TRUE) &&
        !PhEqualStringZ(name->Buffer, L"desktop.ini", TRUE))
    {
        PPH_STRING path;

        path = PhConcatStringRef3(&context->Location->sr, &separator, &name->sr);
        AtpAddStartupEntry(context, name, path, NULL, NULL);
        PhDereferenceObject(path);
    }

    PhDereferenceObject(name);

    return TRUE;
}

VOID AtpReadStartupFolder(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_ ULONG Folder,
    _In_ BOOLEAN Machine
    )
{
    NTSTATUS status;
    static CONST PH_STRINGREF startupSuffix = PH_STRINGREF_INIT(L"\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
    PPH_STRING folderPath;
    HANDLE directoryHandle;

    if (!AtpSetStartupSource(Context, L"", AtpStartupScope(Machine), L"startup_folder", AtStartupImagePath))
        return;

    if (!(folderPath = PhGetKnownLocation(Folder, &startupSuffix, FALSE)))
        return;

    PhMoveReference(&Context->Location, folderPath);

    if (NT_SUCCESS(status = PhCreateFileWin32(
        &directoryHandle,
        PhGetString(Context->Location),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        status = PhEnumDirectoryFile(directoryHandle, NULL, AtpStartupFolderCallback, Context);
        NtClose(directoryHandle);
    }

    AtpStartupUnreadable(Context, status);
}

VOID AtpReadWinlogonValues(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_ BOOLEAN Machine
    )
{
    NTSTATUS status;
    static CONST PH_STRINGREF winlogonKey = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
    static CONST PH_STRINGREF commaSeparator = PH_STRINGREF_INIT(L",");
    static CONST AT_STARTUP_VALUE winlogonValues[] =
    {
        { L"Shell", AtStartupImageCommandLine, TRUE },
        { L"Userinit", AtStartupImageCommandLine, TRUE },
        { L"Taskman", AtStartupImageCommandLine, FALSE },
        { L"System", AtStartupImageCommandLine, TRUE },
        { L"VmApplet", AtStartupImageCommandLine, TRUE },
        { L"AppSetup", AtStartupImageCommandLine, TRUE },
        { L"UIHost", AtStartupImageCommandLine, FALSE },
        { L"GinaDLL", AtStartupImageModule, FALSE },
    };
    HANDLE keyHandle;
    ULONG i;

    if (!AtpSetStartupSource(
        Context,
        Machine ?
        L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon" :
        L"HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        AtpStartupScope(Machine),
        L"winlogon",
        AtStartupImageCommandLine
        ))
    {
        return;
    }

    if (!NT_SUCCESS(status = PhOpenKey(&keyHandle, KEY_QUERY_VALUE, AtpStartupRoot(Machine), &winlogonKey, 0)))
    {
        AtpStartupUnreadable(Context, status);
        return;
    }

    for (i = 0; i < RTL_NUMBER_OF(winlogonValues); i++)
    {
        PPH_STRING name;
        PPH_STRING value;

        if (!(value = PhQueryRegistryStringZ(keyHandle, winlogonValues[i].Name)))
            continue;

        if (PhIsNullOrEmptyString(value))
        {
            PhDereferenceObject(value);
            continue;
        }

        Context->ImageKind = winlogonValues[i].ImageKind;
        name = PhCreateString(winlogonValues[i].Name);

        if (winlogonValues[i].Separated)
            AtpAddSeparatedEntries(Context, name, value, &commaSeparator, NULL);
        else
            AtpAddStartupEntry(Context, name, value, NULL, NULL);

        PhDereferenceObject(name);
        PhDereferenceObject(value);
    }

    NtClose(keyHandle);
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpStartupSubKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\\");
    PAT_STARTUP_CONTEXT context = Context;
    PKEY_BASIC_INFORMATION information = Information;
    PPH_STRING location;
    PPH_STRING name;
    PPH_STRING value;
    PH_STRINGREF nameRef;
    HANDLE keyHandle;

    if (!context)
        return FALSE;

    nameRef.Buffer = information->Name;
    nameRef.Length = information->NameLength;

    if (!NT_SUCCESS(status = PhOpenKey(&keyHandle, KEY_QUERY_VALUE, RootDirectory, &nameRef, 0)))
    {
        AtpStartupUnreadable(context, status);
        return TRUE;
    }

    value = PhQueryRegistryStringZ(keyHandle, context->SubKeyValueName);
    NtClose(keyHandle);

    if (PhIsNullOrEmptyString(value))
    {
        PhClearReference(&value);
        return TRUE;
    }

    name = PhCreateString2(&nameRef);
    location = context->Location;
    context->Location = PhConcatStringRef3(&location->sr, &separator, &nameRef);

    AtpAddStartupEntry(context, name, value, NULL, NULL);

    PhMoveReference(&context->Location, location);

    PhDereferenceObject(name);
    PhDereferenceObject(value);

    return TRUE;
}

VOID AtpReadSubKeyValues(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_ BOOLEAN Machine,
    _In_ PCWSTR SubKey,
    _In_ PCWSTR Location,
    _In_ PCWSTR Kind,
    _In_ PCWSTR ValueName,
    _In_ AT_STARTUP_IMAGE ImageKind
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PH_STRINGREF subKey;

    if (!AtpSetStartupSource(Context, Location, AtpStartupScope(Machine), Kind, ImageKind))
        return;

    Context->SubKeyValueName = ValueName;

    PhInitializeStringRef(&subKey, SubKey);

    status = PhOpenKey(&keyHandle, KEY_ENUMERATE_SUB_KEYS, AtpStartupRoot(Machine), &subKey, 0);

    if (NT_SUCCESS(status))
    {
        status = PhEnumerateKey(keyHandle, KeyBasicInformation, AtpStartupSubKeyCallback, Context);
        NtClose(keyHandle);
    }

    AtpStartupUnreadable(Context, status);
}

VOID AtpReadWinlogonNotify(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    AtpReadSubKeyValues(
        Context,
        TRUE,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify",
        L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify",
        L"winlogon_notify",
        L"DllName",
        AtStartupImageModule
        );
}

VOID AtpReadIfeoDebuggers(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    AtpReadSubKeyValues(
        Context,
        TRUE,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
        L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
        L"ifeo_debugger",
        L"Debugger",
        AtStartupImageCommandLine
        );
    AtpReadSubKeyValues(
        Context,
        TRUE,
        L"Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
        L"HKLM\\Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
        L"ifeo_debugger",
        L"Debugger",
        AtStartupImageCommandLine
        );
}

VOID AtpReadActiveSetup(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    AtpReadSubKeyValues(
        Context,
        TRUE,
        L"Software\\Microsoft\\Active Setup\\Installed Components",
        L"HKLM\\Software\\Microsoft\\Active Setup\\Installed Components",
        L"active_setup",
        L"StubPath",
        AtStartupImageCommandLine
        );
    AtpReadSubKeyValues(
        Context,
        TRUE,
        L"Software\\Wow6432Node\\Microsoft\\Active Setup\\Installed Components",
        L"HKLM\\Software\\Wow6432Node\\Microsoft\\Active Setup\\Installed Components",
        L"active_setup",
        L"StubPath",
        AtStartupImageCommandLine
        );
}

VOID AtpReadPrintMonitors(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    AtpReadSubKeyValues(
        Context,
        TRUE,
        L"System\\CurrentControlSet\\Control\\Print\\Monitors",
        L"HKLM\\System\\CurrentControlSet\\Control\\Print\\Monitors",
        L"print_monitor",
        L"Driver",
        AtStartupImageModule
        );
}

VOID AtpReadAppInitDlls(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_ BOOLEAN Wow64
    )
{
    NTSTATUS status;
    static CONST PH_STRINGREF windowsKey = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows");
    static CONST PH_STRINGREF windowsKeyWow64 = PH_STRINGREF_INIT(L"Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows");
    static CONST PH_STRINGREF separators = PH_STRINGREF_INIT(L" ,;");
    HANDLE keyHandle;
    PPH_STRING name;
    PPH_STRING value;
    ULONG load;
    BOOLEAN enabled;

    if (!AtpSetStartupSource(
        Context,
        Wow64 ?
        L"HKLM\\Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows" :
        L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
        L"machine",
        L"appinit_dll",
        AtStartupImageModule
        ))
    {
        return;
    }

    if (!NT_SUCCESS(status = PhOpenKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        PH_KEY_LOCAL_MACHINE,
        Wow64 ? &windowsKeyWow64 : &windowsKey,
        0
        )))
    {
        AtpStartupUnreadable(Context, status);
        return;
    }

    if (value = PhQueryRegistryStringZ(keyHandle, L"AppInit_DLLs"))
    {
        // The list only runs when LoadAppInit_DLLs says so; reporting a value that is present but
        // switched off without saying so reads as persistence that is in force.
        load = PhQueryRegistryUlongZ(keyHandle, L"LoadAppInit_DLLs");
        enabled = load != ULONG_MAX && load != 0;

        if (!PhIsNullOrEmptyString(value))
        {
            name = PhCreateString(L"AppInit_DLLs");
            AtpAddSeparatedEntries(Context, name, value, &separators, &enabled);
            PhDereferenceObject(name);
        }

        PhDereferenceObject(value);
    }

    NtClose(keyHandle);
}

VOID AtpAddMultiStringEntries(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName
    )
{
    static CONST PH_STRINGREF nullSeparator = PH_STRINGREF_INIT(L"\0");
    PKEY_VALUE_PARTIAL_INFORMATION buffer;
    PPH_STRING name;
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    if (!NT_SUCCESS(PhQueryValueKeyZ(KeyHandle, ValueName, KeyValuePartialInformation, &buffer)))
        return;

    if (buffer->Type != REG_MULTI_SZ && buffer->Type != REG_SZ)
    {
        PhFree(buffer);
        return;
    }

    name = PhCreateString(ValueName);
    remaining.Buffer = (PWCHAR)buffer->Data;
    remaining.Length = buffer->DataLength & ~(sizeof(WCHAR) - 1);

    while (remaining.Length)
    {
        PPH_STRING package;

        PhSplitStringRefEx(&remaining, &nullSeparator, PH_SPLIT_AT_CHAR_SET, &part, &remaining, NULL);

        if (!part.Length)
            continue;

        package = PhCreateString2(&part);
        AtpAddStartupEntry(Context, name, package, NULL, NULL);
        PhDereferenceObject(package);
    }

    PhDereferenceObject(name);
    PhFree(buffer);
}

VOID AtpReadLsaPackages(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    NTSTATUS status;
    static CONST AT_STARTUP_KEY lsaKeys[] =
    {
        { TRUE, L"System\\CurrentControlSet\\Control\\Lsa",
            L"HKLM\\System\\CurrentControlSet\\Control\\Lsa" },
        { TRUE, L"System\\CurrentControlSet\\Control\\Lsa\\OSConfig",
            L"HKLM\\System\\CurrentControlSet\\Control\\Lsa\\OSConfig" },
    };
    static CONST PCWSTR lsaValues[] =
    {
        L"Authentication Packages",
        L"Notification Packages",
        L"Security Packages"
    };
    HANDLE keyHandle;
    PH_STRINGREF subKey;
    ULONG i;
    ULONG j;

    for (i = 0; i < RTL_NUMBER_OF(lsaKeys); i++)
    {
        if (!AtpSetStartupSource(
            Context,
            lsaKeys[i].Location,
            L"machine",
            L"lsa_package",
            AtStartupImageModule
            ))
        {
            return;
        }

        PhInitializeStringRef(&subKey, lsaKeys[i].SubKey);

        if (!NT_SUCCESS(status = PhOpenKey(&keyHandle, KEY_QUERY_VALUE, PH_KEY_LOCAL_MACHINE, &subKey, 0)))
        {
            AtpStartupUnreadable(Context, status);
            continue;
        }

        for (j = 0; j < RTL_NUMBER_OF(lsaValues); j++)
            AtpAddMultiStringEntries(Context, keyHandle, lsaValues[j]);

        NtClose(keyHandle);
    }
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpKnownDllValueCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PAT_STARTUP_CONTEXT context = Context;
    PKEY_VALUE_FULL_INFORMATION information = Information;
    PPH_STRING name;
    PPH_STRING command;

    if (!context)
        return FALSE;

    name = PhCreateStringEx(information->Name, information->NameLength);

    // DllDirectory and DllDirectory32 hold the directory the known DLLs are mapped from, not a
    // module of their own.
    if (PhStartsWithString2(name, L"DllDirectory", TRUE))
    {
        PhDereferenceObject(name);
        return TRUE;
    }

    command = AtpGetRegistryStringData(information);
    AtpAddStartupEntry(context, name, command, NULL, NULL);

    PhClearReference(&command);
    PhDereferenceObject(name);

    return TRUE;
}

VOID AtpReadKnownDlls(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    if (!AtpSetStartupSource(
        Context,
        L"HKLM\\System\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs",
        L"machine",
        L"known_dll",
        AtStartupImageModule
        ))
    {
        return;
    }

    AtpEnumerateValueKey(
        Context,
        TRUE,
        L"System\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs",
        AtpKnownDllValueCallback
        );
}

PPH_STRING AtpResolveClsidModule(
    _In_ PPH_STRING Clsid,
    _Out_opt_ PPH_STRING* FriendlyName
    )
{
    static CONST PH_STRINGREF inprocSuffix = PH_STRINGREF_INIT(L"\\InprocServer32");
    static CONST AT_STARTUP_KEY clsidKeys[] =
    {
        { FALSE, L"Software\\Classes\\CLSID\\", NULL },
        { TRUE, L"Software\\Classes\\CLSID\\", NULL },
        { TRUE, L"Software\\Classes\\Wow6432Node\\CLSID\\", NULL },
    };
    PPH_STRING module = NULL;
    PH_STRINGREF prefix;
    HANDLE keyHandle;
    ULONG i;

    if (FriendlyName)
        *FriendlyName = NULL;

    // A class identifier is a brace-wrapped GUID; anything else does not name a key and is not made
    // into one.
    if (PhIsNullOrEmptyString(Clsid) ||
        Clsid->Buffer[0] != L'{' ||
        Clsid->Buffer[Clsid->Length / sizeof(WCHAR) - 1] != L'}')
    {
        return NULL;
    }

    for (i = 0; i < RTL_NUMBER_OF(clsidKeys); i++)
    {
        PPH_STRING classKey;
        PPH_STRING serverKey;

        PhInitializeStringRef(&prefix, clsidKeys[i].SubKey);
        classKey = PhConcatStringRef2(&prefix, &Clsid->sr);
        serverKey = PhConcatStringRef2(&classKey->sr, &inprocSuffix);

        if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_QUERY_VALUE, AtpStartupRoot(clsidKeys[i].Machine), &serverKey->sr, 0)))
        {
            module = PhQueryRegistryString(keyHandle, NULL);
            NtClose(keyHandle);

            if (PhIsNullOrEmptyString(module))
                PhClearReference(&module);
        }

        if (module && FriendlyName && !*FriendlyName)
        {
            if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_QUERY_VALUE, AtpStartupRoot(clsidKeys[i].Machine), &classKey->sr, 0)))
            {
                *FriendlyName = PhQueryRegistryString(keyHandle, NULL);
                NtClose(keyHandle);

                if (PhIsNullOrEmptyString(*FriendlyName))
                    PhClearReference(FriendlyName);
            }
        }

        PhDereferenceObject(serverKey);
        PhDereferenceObject(classKey);

        if (module)
            break;
    }

    return module;
}

VOID AtpAddClsidEntry(
    _In_ PAT_STARTUP_CONTEXT Context,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING Clsid
    )
{
    PPH_STRING friendlyName = NULL;
    PPH_STRING module;
    AT_STARTUP_IMAGE imageKind;

    module = AtpResolveClsidModule(Clsid, &friendlyName);

    // A class that names no server has no file, and the class identifier is not a module name.
    imageKind = Context->ImageKind;

    if (!module)
        Context->ImageKind = AtStartupImageNone;

    AtpAddStartupEntry(Context, friendlyName ? friendlyName : Name, Clsid, module, NULL);

    Context->ImageKind = imageKind;

    PhClearReference(&friendlyName);
    PhClearReference(&module);
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpShellHookValueCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PAT_STARTUP_CONTEXT context = Context;
    PKEY_VALUE_FULL_INFORMATION information = Information;
    PPH_STRING name;
    PPH_STRING data;

    if (!context)
        return FALSE;

    name = PhCreateStringEx(information->Name, information->NameLength);
    data = AtpGetRegistryStringData(information);

    // Some of these keys name the class in the value name and use the data for a description;
    // others name it in the data.
    if (context->ClsidInData)
    {
        if (!PhIsNullOrEmptyString(data))
            AtpAddClsidEntry(context, name, data);
    }
    else
    {
        AtpAddClsidEntry(context, name, name);
    }

    PhClearReference(&data);
    PhDereferenceObject(name);

    return TRUE;
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpShellHookSubKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\\");
    PAT_STARTUP_CONTEXT context = Context;
    PKEY_BASIC_INFORMATION information = Information;
    PPH_STRING location;
    PPH_STRING name;
    PH_STRINGREF nameRef;

    if (!context)
        return FALSE;

    nameRef.Buffer = information->Name;
    nameRef.Length = information->NameLength;

    name = PhCreateString2(&nameRef);
    location = context->Location;
    context->Location = PhConcatStringRef3(&location->sr, &separator, &nameRef);

    AtpAddClsidEntry(context, name, name);

    PhMoveReference(&context->Location, location);
    PhDereferenceObject(name);

    return TRUE;
}

VOID AtpReadShellHooks(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    static CONST struct
    {
        BOOLEAN Machine;
        BOOLEAN ClsidInData;
        PCWSTR SubKey;
        PCWSTR Location;
    } shellHookKeys[] =
    {
        { TRUE, FALSE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks",
            L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks" },
        { TRUE, FALSE, L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks",
            L"HKLM\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks" },
        { TRUE, FALSE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SharedTaskScheduler",
            L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SharedTaskScheduler" },
        { TRUE, TRUE, L"Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad",
            L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad" },
        { FALSE, FALSE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks",
            L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellExecuteHooks" },
    };
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(shellHookKeys); i++)
    {
        if (!AtpSetStartupSource(
            Context,
            shellHookKeys[i].Location,
            AtpStartupScope(shellHookKeys[i].Machine),
            L"shell_hook",
            AtStartupImageModule
            ))
        {
            return;
        }

        Context->ClsidInData = shellHookKeys[i].ClsidInData;

        AtpEnumerateValueKey(
            Context,
            shellHookKeys[i].Machine,
            shellHookKeys[i].SubKey,
            AtpShellHookValueCallback
            );
    }
}

VOID AtpReadBrowserHelperObjects(
    _In_ PAT_STARTUP_CONTEXT Context
    )
{
    NTSTATUS status;
    static CONST AT_STARTUP_KEY bhoKeys[] =
    {
        { TRUE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects",
            L"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects" },
        { TRUE, L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects",
            L"HKLM\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects" },
        { FALSE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects",
            L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects" },
    };
    HANDLE keyHandle;
    PH_STRINGREF subKey;
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(bhoKeys); i++)
    {
        if (!AtpSetStartupSource(
            Context,
            bhoKeys[i].Location,
            AtpStartupScope(bhoKeys[i].Machine),
            L"browser_helper_object",
            AtStartupImageModule
            ))
        {
            return;
        }

        PhInitializeStringRef(&subKey, bhoKeys[i].SubKey);

        status = PhOpenKey(&keyHandle, KEY_ENUMERATE_SUB_KEYS, AtpStartupRoot(bhoKeys[i].Machine), &subKey, 0);

        if (NT_SUCCESS(status))
        {
            status = PhEnumerateKey(keyHandle, KeyBasicInformation, AtpShellHookSubKeyCallback, Context);
            NtClose(keyHandle);
        }
    }
}

VOID AtListStartupEntries(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_STARTUP_CONTEXT context;
    AT_ROWS entries;
    PVOID structured;

    memset(&context, 0, sizeof(AT_STARTUP_CONTEXT));
    context.Entries = &entries;
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.KindFilter = AtGetArgumentString(Call->Arguments, "kind");
    // Off unless asked for: a catalog lookup hashes the whole image, so verifying a dozen autostart
    // entries is minutes rather than seconds.
    context.Verify = AtJsonGetObjectBoolean(Call->Arguments, "verify");

    structured = PhCreateJsonObject();
    AtInitializeRows(&entries, Call->Arguments);

    AtpReadRunKeys(&context);
    AtpReadStartupFolder(&context, PH_FOLDERID_RoamingAppData, FALSE);
    AtpReadStartupFolder(&context, PH_FOLDERID_ProgramData, TRUE);
    AtpReadWinlogonValues(&context, TRUE);
    AtpReadWinlogonValues(&context, FALSE);
    AtpReadWinlogonNotify(&context);
    AtpReadIfeoDebuggers(&context);
    AtpReadAppInitDlls(&context, FALSE);
    AtpReadAppInitDlls(&context, TRUE);
    AtpReadLsaPackages(&context);
    AtpReadKnownDlls(&context);
    AtpReadPrintMonitors(&context);
    AtpReadShellHooks(&context);
    AtpReadBrowserHelperObjects(&context);
    AtpReadActiveSetup(&context);

    AtAddRows(structured, "entries", &entries);
    PhAddJsonObjectUInt64(structured, "unreadable_count", context.UnreadableCount);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&context.Location);
    PhClearReference(&context.KindFilter);
    PhClearReference(&context.NameContains);
}
