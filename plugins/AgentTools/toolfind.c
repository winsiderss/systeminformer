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

// Who is holding a thing, across every process: the question get_process_handles_detailed cannot
// answer, because it needs to be told the process first. This is what says which process has the
// file that will not delete, or which one still has the registry key or the mutex.
//
// The scan opens every process it can and asks each handle for its name, so it is not free. It
// refuses to run with no filter at all, filters on what is cheap before it names anything, and
// stops on a time budget rather than running for as long as it takes.

#define AT_FIND_DEFAULT_SECONDS 20
#define AT_FIND_MAXIMUM_SECONDS 60

typedef struct _AT_FIND_HANDLES_CONTEXT
{
    AT_ROWS Rows;
    PPH_STRING NameContains;
    PPH_STRING TypeName;
    HANDLE ProcessId;
    BOOLEAN HaveProcessId;
    ULONG64 Deadline;
    ULONG Scanned;
    ULONG Named;
    BOOLEAN TimedOut;
} AT_FIND_HANDLES_CONTEXT, *PAT_FIND_HANDLES_CONTEXT;

VOID AtpAddFoundHandle(
    _In_ PAT_FIND_HANDLES_CONTEXT Context,
    _In_ PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Entry,
    _In_ PPH_STRING TypeName,
    _In_opt_ PPH_STRING ObjectName,
    _In_opt_ PPH_STRING BestName
    )
{
    PVOID row;
    PPH_PROCESS_ITEM processItem;

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "pid", HandleToUlong(Entry->UniqueProcessId));

    // The sequence number goes with the pid so a row can be handed straight to close_handle and
    // be refused if the process has since exited and the pid was reused.
    if (processItem = PhReferenceProcessItem(Entry->UniqueProcessId))
    {
        PhAddJsonObjectUInt64(row, "process_sequence_number", processItem->ProcessSequenceNumber);
        AtJsonAddString(row, "process_name", processItem->ProcessName);
        PhDereferenceObject(processItem);
    }
    else
    {
        AtJsonAddNull(row, "process_sequence_number");
        AtJsonAddNull(row, "process_name");
    }

    AtJsonAddPointer(row, "handle", Entry->HandleValue);
    AtJsonAddString(row, "type", TypeName);
    AtJsonAddString(row, "object_name", ObjectName);
    AtJsonAddString(row, "best_name", BestName);
    AtJsonAddPointer(row, "object_address", Entry->Object);
    AtJsonAddHex(row, "granted_access", Entry->GrantedAccess);

    AtAddRow(&Context->Rows, row);
}

VOID AtpFindHandles(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_FIND_HANDLES_CONTEXT context;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PPH_HASHTABLE processHandles;
    PPH_KEY_VALUE_PAIR pair;
    PVOID structured;
    ULONG64 processId;
    ULONG64 seconds = AT_FIND_DEFAULT_SECONDS;
    NTSTATUS status;
    ULONG i;

    memset(&context, 0, sizeof(AT_FIND_HANDLES_CONTEXT));
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.TypeName = AtGetArgumentString(Call->Arguments, "type_name");
    context.HaveProcessId = AtGetArgumentUInt64(Call->Arguments, "pid", &processId);

    if (context.HaveProcessId)
        context.ProcessId = UlongToHandle((ULONG)processId);

    // A scan with nothing to match would name every handle on the machine and answer a question
    // nobody asked.
    if (!context.NameContains && !context.TypeName && !context.HaveProcessId)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"Give at least one of name_contains, type_name or pid; this searches every handle on the machine."
            );
        PhClearReference(&context.NameContains);
        PhClearReference(&context.TypeName);
        return;
    }

    if (AtGetArgumentUInt64(Call->Arguments, "max_seconds", &seconds))
        seconds = min(max(seconds, 1), AT_FIND_MAXIMUM_SECONDS);

    if (!NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
    {
        AtSetToolStatusError(Result, status, L"Enumerating the handles");
        PhClearReference(&context.NameContains);
        PhClearReference(&context.TypeName);
        return;
    }

    context.Deadline = NtGetTickCount64() + seconds * 1000;

    AtInitializeRows(&context.Rows, Call->Arguments);
    processHandles = PhCreateSimpleHashtable(16);

    for (i = 0; i < handles->NumberOfHandles; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = &handles->Handles[i];
        HANDLE processHandle;
        PPH_STRING typeName = NULL;
        PPH_STRING objectName = NULL;
        PPH_STRING bestName = NULL;

        if (NtGetTickCount64() > context.Deadline)
        {
            context.TimedOut = TRUE;
            break;
        }

        if (context.HaveProcessId && entry->UniqueProcessId != context.ProcessId)
            continue;

        // The type comes from the index alone, with no process opened and no object queried, so
        // a type filter costs nothing and removes almost everything.
        if (context.TypeName)
        {
            PPH_STRING indexName = PhGetObjectTypeIndexName(entry->ObjectTypeIndex);

            if (!indexName)
                continue;

            if (!PhEqualString(indexName, context.TypeName, TRUE))
            {
                PhDereferenceObject(indexName);
                continue;
            }

            PhDereferenceObject(indexName);
        }

        context.Scanned++;

        if (!(processHandle = PhFindItemSimpleHashtable2(processHandles, entry->UniqueProcessId)))
        {
            HANDLE opened = NULL;

            // Duplicating the handle is what names most objects; without that access the entry
            // can still be reported by type and address, so a failure to open is not fatal.
            if (!NT_SUCCESS(PhOpenProcess(&opened, PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, entry->UniqueProcessId)))
                PhOpenProcess(&opened, PROCESS_QUERY_INFORMATION, entry->UniqueProcessId);

            // Cached either way: a process that cannot be opened must not be retried for every
            // one of its handles.
            PhAddItemSimpleHashtable(processHandles, entry->UniqueProcessId, opened);
            processHandle = opened;
        }

        if (!processHandle)
            continue;

        // File handles can block a name query forever; PhGetHandleInformationEx already asks for
        // those through a thread it can abandon, which is why this can run on the server thread.
        if (NT_SUCCESS(PhGetHandleInformationEx(
            processHandle,
            entry->HandleValue,
            entry->ObjectTypeIndex,
            0,
            NULL,
            NULL,
            &typeName,
            &objectName,
            &bestName,
            NULL
            )))
        {
            context.Named++;

            if (!context.NameContains ||
                AtContainsString(objectName, context.NameContains) ||
                AtContainsString(bestName, context.NameContains))
            {
                AtpAddFoundHandle(&context, entry, typeName, objectName, bestName);
            }
        }

        PhClearReference(&typeName);
        PhClearReference(&objectName);
        PhClearReference(&bestName);
    }

    {
        ULONG enumerationKey = 0;

        while (PhEnumHashtable(processHandles, &pair, &enumerationKey))
        {
            if (pair->Value)
                NtClose(pair->Value);
        }
    }

    PhDereferenceObject(processHandles);
    PhFree(handles);

    structured = PhCreateJsonObject();
    AtAddRows(structured, "handles", &context.Rows);
    PhAddJsonObjectUInt64(structured, "scanned", context.Scanned);
    PhAddJsonObjectUInt64(structured, "named", context.Named);
    // A scan that ran out of time has looked at some of the machine, not all of it, and an empty
    // answer from it means nothing.
    PhAddJsonObjectBoolean(structured, "timed_out", context.TimedOut);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);
    PhClearReference(&context.NameContains);
    PhClearReference(&context.TypeName);
}

// The same question for loaded code: which processes have this module mapped. ListDLLs answers it
// for one machine-wide search; this adds the signature, because "which processes loaded this
// unsigned DLL" is the version of the question worth asking.
//
// Verification is by far the most expensive part, and the same DLL is loaded in a hundred
// processes, so a result is looked up once per file and remembered for the rest of the scan.

typedef struct _AT_FIND_MODULES_CONTEXT
{
    AT_ROWS Rows;
    PPH_STRING NameContains;
    BOOLEAN UnsignedOnly;
    BOOLEAN Verify;
    BOOLEAN IncludeMappedFiles;
    PPH_HASHTABLE VerifyCache;
    HANDLE ProcessId;
    PPH_STRING ProcessName;
    ULONG64 ProcessSequenceNumber;
    ULONG64 Deadline;
    ULONG Scanned;
    ULONG Verified;
    BOOLEAN TimedOut;
} AT_FIND_MODULES_CONTEXT, *PAT_FIND_MODULES_CONTEXT;

typedef struct _AT_VERIFY_ENTRY
{
    PPH_STRING FileName;
    VERIFY_RESULT Result;
    PPH_STRING Signer;
} AT_VERIFY_ENTRY, *PAT_VERIFY_ENTRY;

static BOOLEAN NTAPI AtpVerifyCacheCompare(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return PhEqualString(((PAT_VERIFY_ENTRY)Entry1)->FileName, ((PAT_VERIFY_ENTRY)Entry2)->FileName, TRUE);
}

static ULONG NTAPI AtpVerifyCacheHash(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&((PAT_VERIFY_ENTRY)Entry)->FileName->sr, TRUE, PH_STRING_HASH_X65599);
}

// One verification per distinct file for the life of the scan. Without this a machine-wide search
// verifies ntdll.dll once for every process on the machine.
BOOLEAN AtpVerifyModuleCached(
    _In_ PAT_FIND_MODULES_CONTEXT Context,
    _In_ PPH_STRING NativeFileName,
    _Out_ PVERIFY_RESULT VerifyResult,
    _Out_ PPH_STRING* Signer
    )
{
    AT_VERIFY_ENTRY lookup;
    PAT_VERIFY_ENTRY found;
    AT_VERIFY_ENTRY entry;
    PPH_STRING win32FileName;

    *VerifyResult = VrUnknown;
    *Signer = NULL;

    lookup.FileName = NativeFileName;

    if (found = PhFindEntryHashtable(Context->VerifyCache, &lookup))
    {
        *VerifyResult = found->Result;
        PhSetReference(Signer, found->Signer);
        return TRUE;
    }

    // PhVerifyFile takes a Win32 path; the module list carries native ones, and the mismatch is
    // silent - everything reads as unverified.
    if (!(win32FileName = PhGetFileName(NativeFileName)))
        return FALSE;

    memset(&entry, 0, sizeof(AT_VERIFY_ENTRY));
    entry.FileName = PhReferenceObject(NativeFileName);
    entry.Result = PhVerifyFile(win32FileName->Buffer, &entry.Signer);
    PhDereferenceObject(win32FileName);

    Context->Verified++;
    PhAddEntryHashtable(Context->VerifyCache, &entry);

    *VerifyResult = entry.Result;
    PhSetReference(Signer, entry.Signer);

    return TRUE;
}

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpFindModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PAT_FIND_MODULES_CONTEXT context = Context;
    PVOID row;
    VERIFY_RESULT verifyResult = VrUnknown;
    PPH_STRING signer = NULL;
    BOOLEAN verified = FALSE;

    if (!context)
        return FALSE;

    context->Scanned++;

    if (context->NameContains &&
        !AtContainsString(Module->Name, context->NameContains) &&
        !AtContainsString(Module->FileName, context->NameContains))
    {
        return TRUE;
    }

    if (context->Verify && Module->FileName)
        verified = AtpVerifyModuleCached(context, Module->FileName, &verifyResult, &signer);

    if (context->UnsignedOnly && (!verified || verifyResult == VrTrusted))
    {
        PhClearReference(&signer);
        return TRUE;
    }

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "pid", HandleToUlong(context->ProcessId));
    PhAddJsonObjectUInt64(row, "process_sequence_number", context->ProcessSequenceNumber);
    AtJsonAddString(row, "process_name", context->ProcessName);
    AtJsonAddString(row, "name", Module->Name);
    AtJsonAddWin32FileName(row, "file_path", Module->FileName);
    AtJsonAddStringZ(row, "type", AtModuleTypeString(Module->Type));
    AtJsonAddPointer(row, "base_address", Module->BaseAddress);
    PhAddJsonObjectUInt64(row, "size", Module->Size);

    if (context->Verify)
    {
        AtJsonAddStringZ(row, "signature", verified ? AtVerifyResultString(verifyResult) : NULL);
        AtJsonAddString(row, "signer", signer);
    }
    else
    {
        AtJsonAddNull(row, "signature");
        AtJsonAddNull(row, "signer");
    }

    AtAddRow(&context->Rows, row);
    PhClearReference(&signer);

    return TRUE;
}

VOID AtpFindModules(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_FIND_MODULES_CONTEXT context;
    PPH_PROCESS_ITEM* processItems;
    ULONG numberOfProcessItems;
    PVOID structured;
    ULONG64 processId;
    ULONG64 seconds = AT_FIND_DEFAULT_SECONDS;
    BOOLEAN haveProcessId;
    ULONG i;

    memset(&context, 0, sizeof(AT_FIND_MODULES_CONTEXT));
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.UnsignedOnly = AtJsonGetObjectBoolean(Call->Arguments, "unsigned_only");
    context.Verify = context.UnsignedOnly || AtJsonGetObjectBoolean(Call->Arguments, "verify_signatures");
    context.IncludeMappedFiles = AtJsonGetObjectBoolean(Call->Arguments, "include_mapped_files");
    haveProcessId = AtGetArgumentUInt64(Call->Arguments, "pid", &processId);

    if (!context.NameContains && !context.UnsignedOnly && !haveProcessId)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"Give at least one of name_contains, unsigned_only or pid; this walks the modules of every process."
            );
        PhClearReference(&context.NameContains);
        return;
    }

    if (AtGetArgumentUInt64(Call->Arguments, "max_seconds", &seconds))
        seconds = min(max(seconds, 1), AT_FIND_MAXIMUM_SECONDS);

    context.Deadline = NtGetTickCount64() + seconds * 1000;
    context.VerifyCache = PhCreateHashtable(sizeof(AT_VERIFY_ENTRY), AtpVerifyCacheCompare, AtpVerifyCacheHash, 64);

    AtInitializeRows(&context.Rows, Call->Arguments);
    PhEnumProcessItems(&processItems, &numberOfProcessItems);

    for (i = 0; i < numberOfProcessItems; i++)
    {
        PPH_PROCESS_ITEM processItem = processItems[i];

        if (NtGetTickCount64() > context.Deadline)
        {
            context.TimedOut = TRUE;
            break;
        }

        if (haveProcessId && processItem->ProcessId != UlongToHandle((ULONG)processId))
            continue;

        if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
            continue;

        context.ProcessId = processItem->ProcessId;
        context.ProcessName = processItem->ProcessName;
        context.ProcessSequenceNumber = processItem->ProcessSequenceNumber;

        // Mapped images as well as loaded modules, because an image mapped without being loaded
        // is exactly what is worth finding. Mapped data files are left out unless asked for: they
        // are not code, so every one of them is trivially unsigned, and including them buries a
        // machine-wide unsigned_only search under cache and database files.
        PhEnumGenericModules(
            processItem->ProcessId,
            NULL,
            context.IncludeMappedFiles ? (PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES) : PH_ENUM_GENERIC_MAPPED_IMAGES,
            AtpFindModulesCallback,
            &context
            );
    }

    PhDereferenceObjects(processItems, numberOfProcessItems);
    PhFree(processItems);

    {
        PH_HASHTABLE_ENUM_CONTEXT enumContext;
        PAT_VERIFY_ENTRY entry;

        PhBeginEnumHashtable(context.VerifyCache, &enumContext);

        while (entry = PhNextEnumHashtable(&enumContext))
        {
            PhClearReference(&entry->FileName);
            PhClearReference(&entry->Signer);
        }

        PhDereferenceObject(context.VerifyCache);
    }

    structured = PhCreateJsonObject();
    AtAddRows(structured, "modules", &context.Rows);
    PhAddJsonObjectUInt64(structured, "scanned", context.Scanned);
    PhAddJsonObjectUInt64(structured, "files_verified", context.Verified);
    PhAddJsonObjectBoolean(structured, "timed_out", context.TimedOut);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);
    PhClearReference(&context.NameContains);
}

// Who is using one named file, which is the question behind "why can this not be deleted". Two
// different answers, because there are two ways to be using a file and neither implies the other:
// a process can hold a handle to it, and a process can have it mapped into its address space. A
// running executable's own image is usually the second without the first.
//
// The filesystem answers the handle half itself. The mapped half is a walk of every process's
// modules, the same walk find_modules does.

typedef struct _AT_FILE_MAPPED_CONTEXT
{
    PVOID Array;
    PPH_STRING Win32FileName;
    PPH_STRING BaseName;
    PPH_PROCESS_ITEM ProcessItem;
    ULONG Count;
} AT_FILE_MAPPED_CONTEXT, *PAT_FILE_MAPPED_CONTEXT;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpFileMappedCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PAT_FILE_MAPPED_CONTEXT context = Context;
    PVOID entry;

    if (!context)
        return FALSE;

    // Skip, not stop: returning FALSE ends the walk for the whole process, and a module with no
    // file name is common enough that it would end most of them at the first entry.
    if (!Module->FileName)
        return TRUE;

    // The two sides do not agree on how a path is spelled - one comes from a file handle, the
    // other from a module list - so the file name is compared first because it is cheap and
    // almost always decides, and only a match pays for converting both to their Win32 form.
    {
        PPH_STRING baseName = PhGetBaseName(Module->FileName);
        BOOLEAN sameName;

        if (!baseName)
            return TRUE;

        sameName = PhEqualString(baseName, context->BaseName, TRUE);
        PhDereferenceObject(baseName);

        if (!sameName)
            return TRUE;
    }

    {
        PPH_STRING win32FileName = PhGetFileName(Module->FileName);
        BOOLEAN samePath;

        if (!win32FileName)
            return TRUE;

        samePath = PhEqualString(win32FileName, context->Win32FileName, TRUE);
        PhDereferenceObject(win32FileName);

        if (!samePath)
            return TRUE;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "pid", HandleToUlong(context->ProcessItem->ProcessId));
    PhAddJsonObjectUInt64(entry, "process_sequence_number", context->ProcessItem->ProcessSequenceNumber);
    AtJsonAddString(entry, "process_name", context->ProcessItem->ProcessName);
    AtJsonAddStringZ(entry, "type", AtModuleTypeString(Module->Type));
    AtJsonAddPointer(entry, "base_address", Module->BaseAddress);

    PhAddJsonArrayObject(context->Array, entry);
    context->Count++;

    return TRUE;
}

VOID AtpAddFileHandleUsers(
    _In_ PVOID Object,
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN Supported
    )
{
    PFILE_PROCESS_IDS_USING_FILE_INFORMATION processIds;
    PVOID array;
    ULONG i;

    *Supported = FALSE;

    if (!NT_SUCCESS(PhGetProcessIdsUsingFile(FileHandle, &processIds)))
    {
        // Not every filesystem answers this. Saying nobody has the file open would be a different
        // claim from saying nobody could be asked.
        AtJsonAddNull(Object, "handle_users");
        return;
    }

    *Supported = TRUE;
    array = PhCreateJsonArray();

    for (i = 0; i < processIds->NumberOfProcessIdsInList; i++)
    {
        HANDLE processId = (HANDLE)processIds->ProcessIdList[i];
        PPH_PROCESS_ITEM processItem;
        PVOID entry;

        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "pid", HandleToUlong(processId));

        if (processItem = PhReferenceProcessItem(processId))
        {
            PhAddJsonObjectUInt64(entry, "process_sequence_number", processItem->ProcessSequenceNumber);
            AtJsonAddString(entry, "process_name", processItem->ProcessName);
            PhDereferenceObject(processItem);
        }
        else
        {
            AtJsonAddNull(entry, "process_sequence_number");
            AtJsonAddNull(entry, "process_name");
        }

        PhAddJsonArrayObject(array, entry);
    }

    PhAddJsonObjectValue(Object, "handle_users", array);
    PhFree(processIds);
}

VOID AtpGetFileUsers(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING path;
    PPH_STRING nativeFileName = NULL;
    HANDLE fileHandle;
    NTSTATUS status;
    PVOID structured;
    BOOLEAN supported = FALSE;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        return;
    }

    // Opened for attributes only and shared every way, so asking who has the file does not itself
    // become another reason the file is in use, and so a file open for exclusive write still
    // answers.
    status = PhCreateFileWin32(
        &fileHandle,
        path->Buffer,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Opening the file");
        PhDereferenceObject(path);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);

    AtpAddFileHandleUsers(structured, fileHandle, &supported);
    PhAddJsonObjectBoolean(structured, "handle_users_supported", supported);

    // The name the module list carries is the native one, and it comes from the handle rather
    // than from the argument so that a Win32 path, a relative path and a native path all match.
    PhGetFileHandleName(fileHandle, &nativeFileName);
    NtClose(fileHandle);

    if (!nativeFileName)
        nativeFileName = PhReferenceObject(path);

    if (AtJsonGetObjectBoolean(Call->Arguments, "skip_mapped"))
    {
        AtJsonAddNull(structured, "mapped_users");
    }
    else
    {
        AT_FILE_MAPPED_CONTEXT context;
        PPH_PROCESS_ITEM* processItems;
        ULONG numberOfProcessItems;
        ULONG i;

        memset(&context, 0, sizeof(AT_FILE_MAPPED_CONTEXT));
        context.Array = PhCreateJsonArray();
        context.Win32FileName = PhGetFileName(nativeFileName);
        context.BaseName = PhGetBaseName(nativeFileName);

        if (!context.Win32FileName || !context.BaseName)
        {
            PhClearReference(&context.Win32FileName);
            PhClearReference(&context.BaseName);
            AtJsonAddNull(structured, "mapped_users");
            PhFreeJsonObject(context.Array);
            goto FinishExit;
        }

        PhEnumProcessItems(&processItems, &numberOfProcessItems);

        for (i = 0; i < numberOfProcessItems; i++)
        {
            if (!PH_IS_REAL_PROCESS_ID(processItems[i]->ProcessId))
                continue;

            context.ProcessItem = processItems[i];

            PhEnumGenericModules(
                processItems[i]->ProcessId,
                NULL,
                PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES,
                AtpFileMappedCallback,
                &context
                );
        }

        PhDereferenceObjects(processItems, numberOfProcessItems);
        PhFree(processItems);

        PhAddJsonObjectValue(structured, "mapped_users", context.Array);
        PhClearReference(&context.Win32FileName);
        PhClearReference(&context.BaseName);
    }

FinishExit:

    AtJsonAddString(structured, "native_path", nativeFileName);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&nativeFileName);
    PhDereferenceObject(path);
}

// The object namespace, the tree the kernel keeps its named objects in and that nothing on a
// normal machine shows you. What lives here: the device objects drivers publish, the section
// objects shared memory is built on, the mutexes an installer uses to notice a second copy of
// itself, and the symbolic links that make C: mean a volume.
//
// Directories are listed one level at a time by default, because \GLOBAL?? alone has thousands of
// entries and recursing the whole namespace answers no question anyone asked.

#define AT_OBJECT_MAX_DEPTH 8

typedef struct _AT_OBJECT_DIRECTORY_CONTEXT
{
    AT_ROWS Rows;
    PPH_STRING TypeName;
    PPH_STRING NameContains;
    ULONG MaximumDepth;
    ULONG Depth;
    PPH_STRING Path;
    ULONG64 Deadline;
    BOOLEAN TimedOut;
    BOOLEAN ResolveLinks;
} AT_OBJECT_DIRECTORY_CONTEXT, *PAT_OBJECT_DIRECTORY_CONTEXT;

NTSTATUS AtpEnumObjectDirectory(
    _In_ PAT_OBJECT_DIRECTORY_CONTEXT Context,
    _In_ PPH_STRING Path
    );

_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
NTSTATUS NTAPI AtpObjectDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    PAT_OBJECT_DIRECTORY_CONTEXT context = Context;
    PPH_STRING childPath;
    BOOLEAN isDirectory;
    PVOID row;

    if (!context)
        return STATUS_NO_MORE_ENTRIES;

    // STATUS_NO_MORE_ENTRIES is the only status PhEnumDirectoryObjects stops on; anything else and
    // it keeps calling back over the rest of the directory.
    if (NtGetTickCount64() > context->Deadline)
    {
        context->TimedOut = TRUE;
        return STATUS_NO_MORE_ENTRIES;
    }

    isDirectory = PhEqualStringRef2(TypeName, L"Directory", TRUE);

    // The parent path already ends in a separator only when it is the root.
    if (PhEqualString2(context->Path, L"\\", FALSE))
        childPath = PhConcatStringRef2(&context->Path->sr, Name);
    else
    {
        static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\\");

        childPath = PhConcatStringRef3(&context->Path->sr, &separator, Name);
    }

    if ((!context->TypeName || PhEqualStringRef(TypeName, &context->TypeName->sr, TRUE)) &&
        (!context->NameContains || AtContainsString(childPath, context->NameContains)))
    {
        row = PhCreateJsonObject();
        AtJsonAddStringRef(row, "name", Name);
        AtJsonAddString(row, "path", childPath);
        AtJsonAddStringRef(row, "type", TypeName);
        PhAddJsonObjectUInt64(row, "depth", context->Depth);

        // A symbolic link is only interesting for what it points at; \GLOBAL?? is mostly links.
        if (context->ResolveLinks && PhEqualStringRef2(TypeName, L"SymbolicLink", TRUE))
        {
            PPH_STRING target = NULL;

            if (NT_SUCCESS(PhQuerySymbolicLinkObject(&target, RootDirectory, Name)))
            {
                AtJsonAddString(row, "target", target);
                PhClearReference(&target);
            }
            else
            {
                AtJsonAddNull(row, "target");
            }
        }
        else
        {
            AtJsonAddNull(row, "target");
        }

        AtAddRow(&context->Rows, row);
    }

    if (isDirectory && context->Depth + 1 < context->MaximumDepth)
    {
        PPH_STRING parentPath = context->Path;

        context->Depth++;
        context->Path = childPath;

        AtpEnumObjectDirectory(context, childPath);

        context->Path = parentPath;
        context->Depth--;
    }

    PhDereferenceObject(childPath);

    return STATUS_SUCCESS;
}

NTSTATUS AtpEnumObjectDirectory(
    _In_ PAT_OBJECT_DIRECTORY_CONTEXT Context,
    _In_ PPH_STRING Path
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhOpenDirectoryObject(&directoryHandle, DIRECTORY_QUERY, NULL, &Path->sr);

    if (!NT_SUCCESS(status))
        return status;

    status = PhEnumDirectoryObjects(directoryHandle, AtpObjectDirectoryCallback, Context);

    NtClose(directoryHandle);

    return status;
}

VOID AtpListObjectDirectory(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_OBJECT_DIRECTORY_CONTEXT context;
    PPH_STRING path;
    PVOID structured;
    PVOID resolveMember;
    ULONG64 depth = 1;
    ULONG64 seconds = AT_FIND_DEFAULT_SECONDS;
    NTSTATUS status;

    memset(&context, 0, sizeof(AT_OBJECT_DIRECTORY_CONTEXT));

    if (!(path = AtGetArgumentString(Call->Arguments, "path")))
        path = PhCreateString(L"\\");

    context.TypeName = AtGetArgumentString(Call->Arguments, "type_name");
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.MaximumDepth = 1;
    context.ResolveLinks = TRUE;

    if (resolveMember = AtJsonGetObjectMember(Call->Arguments, "resolve_links", PH_JSON_OBJECT_TYPE_BOOLEAN))
        context.ResolveLinks = AtJsonGetObjectBoolean(Call->Arguments, "resolve_links");

    if (AtGetArgumentUInt64(Call->Arguments, "depth", &depth))
        context.MaximumDepth = (ULONG)min(max(depth, 1), AT_OBJECT_MAX_DEPTH);

    if (AtGetArgumentUInt64(Call->Arguments, "max_seconds", &seconds))
        seconds = min(max(seconds, 1), AT_FIND_MAXIMUM_SECONDS);

    context.Deadline = NtGetTickCount64() + seconds * 1000;
    context.Path = path;

    AtInitializeRows(&context.Rows, Call->Arguments);

    status = AtpEnumObjectDirectory(&context, path);

    if (!NT_SUCCESS(status) && status != STATUS_TIMEOUT && context.Rows.Rows->Count == 0)
    {
        AtSetToolStatusError(Result, status, L"Opening the object directory");
        AtDeleteRows(&context.Rows);
        PhClearReference(&context.TypeName);
        PhClearReference(&context.NameContains);
        PhDereferenceObject(path);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    AtAddRows(structured, "objects", &context.Rows);
    PhAddJsonObjectBoolean(structured, "timed_out", context.TimedOut);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);
    PhClearReference(&context.TypeName);
    PhClearReference(&context.NameContains);
    PhDereferenceObject(path);
}

// A single named object, once list_object_directory or find_handles has produced its path. What a
// listing cannot say: how many references the object has, what its security descriptor allows, and
// the state only the object's own query call knows - whether a mutex is held and by which thread,
// whether an event is signalled, how big a section is and what image backs it.
//
// Device, file, ALPC port and filter port objects are described but never opened. Opening a device
// object is a real I/O open with whatever side effects the driver decides; a read tool asking a
// question must not be one of the things that happens to the machine.

typedef enum _AT_OBJECT_KIND
{
    AtObjectKindOther,
    AtObjectKindDirectory,
    AtObjectKindSymbolicLink,
    AtObjectKindMutant,
    AtObjectKindEvent,
    AtObjectKindSemaphore,
    AtObjectKindTimer,
    AtObjectKindSection,
    AtObjectKindJob,
    AtObjectKindKeyedEvent,
    AtObjectKindIoCompletion,
    AtObjectKindDriver
} AT_OBJECT_KIND;

typedef struct _AT_OBJECT_LOOKUP_CONTEXT
{
    PH_STRINGREF Name;
    PPH_STRING TypeName;
} AT_OBJECT_LOOKUP_CONTEXT, *PAT_OBJECT_LOOKUP_CONTEXT;

AT_OBJECT_KIND AtpObjectKindFromTypeName(
    _In_opt_ PPH_STRING TypeName
    )
{
    if (PhIsNullOrEmptyString(TypeName))
        return AtObjectKindOther;

    if (PhEqualString2(TypeName, L"Directory", TRUE))
        return AtObjectKindDirectory;
    if (PhEqualString2(TypeName, L"SymbolicLink", TRUE))
        return AtObjectKindSymbolicLink;
    if (PhEqualString2(TypeName, L"Mutant", TRUE))
        return AtObjectKindMutant;
    if (PhEqualString2(TypeName, L"Event", TRUE))
        return AtObjectKindEvent;
    if (PhEqualString2(TypeName, L"Semaphore", TRUE))
        return AtObjectKindSemaphore;
    if (PhEqualString2(TypeName, L"Timer", TRUE) || PhEqualString2(TypeName, L"IRTimer", TRUE))
        return AtObjectKindTimer;
    if (PhEqualString2(TypeName, L"Section", TRUE))
        return AtObjectKindSection;
    if (PhEqualString2(TypeName, L"Job", TRUE))
        return AtObjectKindJob;
    if (PhEqualString2(TypeName, L"KeyedEvent", TRUE))
        return AtObjectKindKeyedEvent;
    if (PhEqualString2(TypeName, L"IoCompletion", TRUE))
        return AtObjectKindIoCompletion;
    if (PhEqualString2(TypeName, L"Driver", TRUE))
        return AtObjectKindDriver;

    return AtObjectKindOther;
}

ACCESS_MASK AtpObjectQueryAccess(
    _In_ AT_OBJECT_KIND Kind
    )
{
    switch (Kind)
    {
    case AtObjectKindDirectory:
        return DIRECTORY_QUERY;
    case AtObjectKindSymbolicLink:
        return SYMBOLIC_LINK_QUERY;
    case AtObjectKindMutant:
        return MUTANT_QUERY_STATE;
    case AtObjectKindEvent:
        return EVENT_QUERY_STATE;
    case AtObjectKindSemaphore:
        return SEMAPHORE_QUERY_STATE;
    case AtObjectKindTimer:
        return TIMER_QUERY_STATE;
    case AtObjectKindSection:
        return SECTION_QUERY;
    case AtObjectKindJob:
        return JOB_OBJECT_QUERY;
    case AtObjectKindIoCompletion:
        return IO_COMPLETION_QUERY_STATE;
    }

    return 0;
}

NTSTATUS AtpOpenNamespaceObject(
    _Out_ PHANDLE Handle,
    _In_ AT_OBJECT_KIND Kind,
    _In_ PPH_STRING Path,
    _In_ ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    *Handle = NULL;

    if (Kind == AtObjectKindDriver)
        return PhOpenDriver(Handle, DesiredAccess, NULL, &Path->sr);

    if (!PhStringRefToUnicodeString(&Path->sr, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    switch (Kind)
    {
    case AtObjectKindDirectory:
        status = NtOpenDirectoryObject(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindSymbolicLink:
        status = NtOpenSymbolicLinkObject(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindMutant:
        status = NtOpenMutant(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindEvent:
        status = NtOpenEvent(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindSemaphore:
        status = NtOpenSemaphore(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindTimer:
        status = NtOpenTimer(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindSection:
        status = NtOpenSection(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindJob:
        status = NtOpenJobObject(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindKeyedEvent:
        status = NtOpenKeyedEvent(Handle, DesiredAccess, &objectAttributes);
        break;
    case AtObjectKindIoCompletion:
        status = NtOpenIoCompletion(Handle, DesiredAccess, &objectAttributes);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    return status;
}

_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
NTSTATUS NTAPI AtpObjectLookupCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    PAT_OBJECT_LOOKUP_CONTEXT context = Context;

    if (!context)
        return STATUS_NO_MORE_ENTRIES;

    if (!PhEqualStringRef(Name, &context->Name, TRUE))
        return STATUS_SUCCESS;

    context->TypeName = PhCreateString2(TypeName);

    return STATUS_NO_MORE_ENTRIES;
}

// The type decides which open call and which query call apply, so it has to be known before anything
// else. The caller can say, and usually can because a listing just told them; otherwise the parent
// directory is asked about this one name.
PPH_STRING AtpLookupObjectTypeName(
    _In_ PPH_STRING Path,
    _Out_ PNTSTATUS DirectoryStatus
    )
{
    AT_OBJECT_LOOKUP_CONTEXT context;
    PH_STRINGREF directoryPart;
    PH_STRINGREF namePart;
    PPH_STRING directoryName;
    HANDLE directoryHandle;

    *DirectoryStatus = STATUS_SUCCESS;

    if (PhEqualString2(Path, L"\\", FALSE))
        return PhCreateString(L"Directory");

    if (!PhSplitStringRefAtLastChar(&Path->sr, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &namePart))
    {
        *DirectoryStatus = STATUS_OBJECT_PATH_INVALID;
        return NULL;
    }

    memset(&context, 0, sizeof(AT_OBJECT_LOOKUP_CONTEXT));
    context.Name = namePart;

    directoryName = directoryPart.Length != 0 ? PhCreateString2(&directoryPart) : PhCreateString(L"\\");

    *DirectoryStatus = PhOpenDirectoryObject(&directoryHandle, DIRECTORY_QUERY, NULL, &directoryName->sr);

    if (NT_SUCCESS(*DirectoryStatus))
    {
        PhEnumDirectoryObjects(directoryHandle, AtpObjectLookupCallback, &context);
        NtClose(directoryHandle);
    }

    PhDereferenceObject(directoryName);

    return context.TypeName;
}

VOID AtpAddMutantDetails(
    _In_ PVOID Structured,
    _In_ HANDLE Handle
    )
{
    MUTANT_BASIC_INFORMATION basicInfo;
    MUTANT_OWNER_INFORMATION ownerInfo;
    PVOID details;

    if (!NT_SUCCESS(NtQueryMutant(Handle, MutantBasicInformation, &basicInfo, sizeof(basicInfo), NULL)))
        return;

    details = PhCreateJsonObject();
    PhAddJsonObjectInt64(details, "count", basicInfo.CurrentCount);
    PhAddJsonObjectBoolean(details, "owned_by_caller", !!basicInfo.OwnedByCaller);
    PhAddJsonObjectBoolean(details, "abandoned", !!basicInfo.AbandonedState);

    // The owner is the interesting half: a held mutex names the thread that is holding up everyone
    // else waiting on it. It is only reported while the mutex is actually owned.
    if (NT_SUCCESS(NtQueryMutant(Handle, MutantOwnerInformation, &ownerInfo, sizeof(ownerInfo), NULL)) &&
        ownerInfo.ClientId.UniqueProcess)
    {
        PVOID owner = PhCreateJsonObject();
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(ownerInfo.ClientId.UniqueProcess))
        {
            AtFillProcessIdentity(owner, processItem);
            PhDereferenceObject(processItem);
        }
        else
        {
            PhAddJsonObjectUInt64(owner, "pid", HandleToUlong(ownerInfo.ClientId.UniqueProcess));
        }

        PhAddJsonObjectUInt64(owner, "tid", HandleToUlong(ownerInfo.ClientId.UniqueThread));
        PhAddJsonObjectValue(details, "owner", owner);
    }
    else
    {
        AtJsonAddNull(details, "owner");
    }

    PhAddJsonObjectValue(Structured, "mutant", details);
}

VOID AtpAddSectionDetails(
    _In_ PVOID Structured,
    _In_ HANDLE Handle
    )
{
    SECTION_BASIC_INFORMATION basicInfo;
    SECTION_IMAGE_INFORMATION imageInfo;
    BOOLEAN haveImage;

    if (!NT_SUCCESS(NtQuerySection(Handle, SectionBasicInformation, &basicInfo, sizeof(basicInfo), NULL)))
        return;

    haveImage = FlagOn(basicInfo.AllocationAttributes, SEC_IMAGE) &&
        NT_SUCCESS(NtQuerySection(Handle, SectionImageInformation, &imageInfo, sizeof(imageInfo), NULL));

    // The backing file name needs the driver, so a named section reports one only through
    // get_handle_details.
    AtAddSectionInfo(Structured, &basicInfo, haveImage ? &imageInfo : NULL, NULL);
}

VOID AtpAddJobDetails(
    _In_ PVOID Structured,
    _In_ HANDLE Handle
    )
{
    PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;
    PVOID details;
    PVOID array;
    ULONG i;

    if (!NT_SUCCESS(PhGetJobProcessIdList(Handle, &processIdList)))
        return;

    details = PhCreateJsonObject();
    PhAddJsonObjectUInt64(details, "assigned_process_count", processIdList->NumberOfAssignedProcesses);

    array = PhCreateJsonArray();

    for (i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
    {
        PVOID row = PhCreateJsonObject();
        HANDLE processId = (HANDLE)processIdList->ProcessIdList[i];
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(processId))
        {
            AtFillProcessIdentity(row, processItem);
            PhDereferenceObject(processItem);
        }
        else
        {
            PhAddJsonObjectUInt64(row, "pid", HandleToUlong(processId));
        }

        PhAddJsonArrayObject(array, row);
    }

    PhAddJsonObjectValue(details, "processes", array);
    PhAddJsonObjectValue(Structured, "job", details);
    PhFree(processIdList);
}

VOID AtpAddDriverDetails(
    _In_ PVOID Structured,
    _In_ HANDLE Handle
    )
{
    PVOID details;
    PPH_STRING string;

    details = PhCreateJsonObject();

    if (NT_SUCCESS(PhGetDriverName(Handle, &string)))
    {
        AtJsonAddString(details, "name", string);
        PhDereferenceObject(string);
    }
    else
    {
        AtJsonAddNull(details, "name");
    }

    if (NT_SUCCESS(PhGetDriverImageFileName(Handle, &string)))
    {
        AtJsonAddString(details, "image_file_name", string);
        AtJsonAddWin32FileName(details, "image_path", string);
        PhDereferenceObject(string);
    }
    else
    {
        AtJsonAddNull(details, "image_file_name");
        AtJsonAddNull(details, "image_path");
    }

    if (NT_SUCCESS(PhGetDriverServiceKeyName(Handle, &string)))
    {
        AtJsonAddString(details, "service_key_name", string);
        PhDereferenceObject(string);
    }
    else
    {
        AtJsonAddNull(details, "service_key_name");
    }

    PhAddJsonObjectValue(Structured, "driver", details);
}

VOID AtpAddObjectDetails(
    _In_ PVOID Structured,
    _In_ AT_OBJECT_KIND Kind,
    _In_ HANDLE Handle
    )
{
    switch (Kind)
    {
    case AtObjectKindMutant:
        AtpAddMutantDetails(Structured, Handle);
        break;
    case AtObjectKindEvent:
        {
            EVENT_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(NtQueryEvent(Handle, EventBasicInformation, &basicInfo, sizeof(basicInfo), NULL)))
            {
                PVOID details = PhCreateJsonObject();

                AtJsonAddStringZ(details, "event_type",
                    basicInfo.EventType == NotificationEvent ? L"notification" : L"synchronization");
                PhAddJsonObjectBoolean(details, "signaled", !!basicInfo.EventState);
                PhAddJsonObjectValue(Structured, "event", details);
            }
        }
        break;
    case AtObjectKindSemaphore:
        {
            SEMAPHORE_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(NtQuerySemaphore(Handle, SemaphoreBasicInformation, &basicInfo, sizeof(basicInfo), NULL)))
            {
                PVOID details = PhCreateJsonObject();

                PhAddJsonObjectInt64(details, "count", basicInfo.CurrentCount);
                PhAddJsonObjectInt64(details, "maximum_count", basicInfo.MaximumCount);
                PhAddJsonObjectValue(Structured, "semaphore", details);
            }
        }
        break;
    case AtObjectKindTimer:
        {
            TIMER_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(NtQueryTimer(Handle, TimerBasicInformation, &basicInfo, sizeof(basicInfo), NULL)))
            {
                PVOID details = PhCreateJsonObject();

                PhAddJsonObjectBoolean(details, "signaled", !!basicInfo.TimerState);

                // Remaining time counts down as a negative interval and means nothing once the timer
                // has signalled.
                if (!basicInfo.TimerState && basicInfo.RemainingTime.QuadPart < 0)
                    AtJsonAddDuration(details, "remaining", (ULONG64)-basicInfo.RemainingTime.QuadPart);
                else
                    AtJsonAddNull(details, "remaining");

                PhAddJsonObjectValue(Structured, "timer", details);
            }
        }
        break;
    case AtObjectKindSection:
        AtpAddSectionDetails(Structured, Handle);
        break;
    case AtObjectKindJob:
        AtpAddJobDetails(Structured, Handle);
        break;
    case AtObjectKindDriver:
        AtpAddDriverDetails(Structured, Handle);
        break;
    }
}

VOID AtpGetObjectInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST ULONG attributeFlags[] =
    {
        OBJ_INHERIT, OBJ_PERMANENT, OBJ_EXCLUSIVE, OBJ_CASE_INSENSITIVE,
        OBJ_OPENIF, OBJ_OPENLINK, OBJ_KERNEL_HANDLE, OBJ_FORCE_ACCESS_CHECK
    };
    static CONST PWSTR attributeNames[] =
    {
        L"inherit", L"permanent", L"exclusive", L"case_insensitive",
        L"open_if", L"open_link", L"kernel_handle", L"force_access_check"
    };
    NTSTATUS status;
    NTSTATUS directoryStatus = STATUS_SUCCESS;
    PPH_STRING path;
    PPH_STRING typeName;
    PPH_STRING target = NULL;
    AT_OBJECT_KIND kind;
    ACCESS_MASK queryAccess;
    HANDLE objectHandle = NULL;
    PVOID structured;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required");
        return;
    }

    if (!(typeName = AtGetArgumentString(Call->Arguments, "type_name")))
        typeName = AtpLookupObjectTypeName(path, &directoryStatus);

    kind = AtpObjectKindFromTypeName(typeName);
    queryAccess = AtpObjectQueryAccess(kind);

    // READ_CONTROL is what the security descriptor needs and the type access is what the state needs;
    // ask for both, then for each alone, so one being refused does not cost the other.
    status = AtpOpenNamespaceObject(&objectHandle, kind, path, queryAccess | READ_CONTROL);

    if (!NT_SUCCESS(status) && queryAccess)
        status = AtpOpenNamespaceObject(&objectHandle, kind, path, queryAccess);

    if (!NT_SUCCESS(status))
        status = AtpOpenNamespaceObject(&objectHandle, kind, path, READ_CONTROL);

    if (kind == AtObjectKindSymbolicLink)
        PhQuerySymbolicLinkObject(&target, NULL, &path->sr);

    // Nothing opened, no link target and no type: either the parent directory could not be listed -
    // \\Driver needs elevation for that - or the name is genuinely not there. Those are different
    // answers and the caller acts differently on each.
    if (!objectHandle && !target && PhIsNullOrEmptyString(typeName))
    {
        if (!NT_SUCCESS(directoryStatus))
            AtSetToolStatusError(Result, directoryStatus, L"Listing the parent directory");
        else
            AtSetToolStatusError(Result, STATUS_OBJECT_NAME_NOT_FOUND, L"Finding the object");
        PhClearReference(&typeName);
        PhDereferenceObject(path);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    AtJsonAddString(structured, "type", typeName);
    PhAddJsonObjectBoolean(structured, "opened", !!objectHandle);

    if (objectHandle)
    {
        OBJECT_BASIC_INFORMATION basicInfo;
        PPH_STRING sddl;
        PPH_STRING queriedTypeName;

        AtJsonAddNull(structured, "open_error");

        if (NT_SUCCESS(PhGetHandleInformation(
            NtCurrentProcess(),
            objectHandle,
            ULONG_MAX,
            &basicInfo,
            &queriedTypeName,
            NULL,
            NULL
            )))
        {
            // The type the object itself reports, rather than the one the directory listing gave.
            if (!PhIsNullOrEmptyString(queriedTypeName))
                AtJsonAddString(structured, "type", queriedTypeName);

            PhClearReference(&queriedTypeName);

            // Measured against a mutex held by one known process: the handle this call opened is
            // not in the count, so this is what everything else is holding.
            PhAddJsonObjectUInt64(structured, "handle_count", basicInfo.HandleCount);
            PhAddJsonObjectUInt64(structured, "pointer_count", basicInfo.PointerCount);
            PhAddJsonObjectUInt64(structured, "paged_pool_charge", basicInfo.PagedPoolCharge);
            PhAddJsonObjectUInt64(structured, "non_paged_pool_charge", basicInfo.NonPagedPoolCharge);
            AtJsonAddHex(structured, "granted_access", basicInfo.GrantedAccess);
            AtJsonAddFlagStrings(structured, "attributes", basicInfo.Attributes,
                attributeFlags, (CONST PWSTR*)attributeNames, RTL_NUMBER_OF(attributeFlags));
        }

        if (NT_SUCCESS(PhGetObjectSecurityDescriptorAsString(objectHandle, &sddl)))
        {
            AtJsonAddString(structured, "security_descriptor", sddl);
            PhDereferenceObject(sddl);
        }
        else
        {
            AtJsonAddNull(structured, "security_descriptor");
        }

        AtpAddObjectDetails(structured, kind, objectHandle);
        NtClose(objectHandle);
    }
    else
    {
        if (kind == AtObjectKindOther)
        {
            AtJsonAddStringZ(structured, "open_error", L"objects of this type are not opened by this tool");
        }
        else
        {
            PPH_STRING message;

            message = PhGetStatusMessage(status, 0);
            AtJsonAddString(structured, "open_error", message);
            PhClearReference(&message);
        }

        AtJsonAddNull(structured, "security_descriptor");
    }

    if (kind == AtObjectKindSymbolicLink)
    {
        AtJsonAddString(structured, "target", target);
        PhClearReference(&target);
    }

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    PhClearReference(&typeName);
    PhDereferenceObject(path);
}

VOID AtFindInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionFindHandles:
        AtpFindHandles(Call, Result);
        break;
    case AtActionFindModules:
        AtpFindModules(Call, Result);
        break;
    case AtActionGetFileUsers:
        AtpGetFileUsers(Call, Result);
        break;
    case AtActionListObjectDirectory:
        AtpListObjectDirectory(Call, Result);
        break;
    case AtActionGetObjectInfo:
        AtpGetObjectInfo(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
