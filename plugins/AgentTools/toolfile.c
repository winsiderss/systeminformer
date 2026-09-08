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

// What the file system knows about a file, as opposed to what is inside it. The two things here that
// are not on any properties dialog: the alternate data streams, which are where a file keeps content
// nothing shows by default, and the Mark of the Web, which is how Windows remembers that a file was
// downloaded and from where.

VOID AtpTrimTrailingNewline(
    _Inout_opt_ PPH_STRING String
    )
{
    if (!String)
        return;

    while (String->Length >= sizeof(WCHAR))
    {
        WCHAR last = String->Buffer[String->Length / sizeof(WCHAR) - 1];

        if (last != L'\r' && last != L'\n')
            break;

        // The string is this function's alone until it is handed over, so shortening it in place is
        // the one legitimate window to mutate a PH_STRING.
        String->Length -= sizeof(WCHAR);
        String->Buffer[String->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
}

PCWSTR AtpMotwZoneString(
    _In_ PH_MOTW_ZONE_ID ZoneId
    )
{
    switch (ZoneId)
    {
    case PhMotwZoneIdLocalComputer:
        return L"local_computer";
    case PhMotwZoneIdLocalIntranet:
        return L"local_intranet";
    case PhMotwZoneIdTrustedSites:
        return L"trusted_sites";
    case PhMotwZoneIdInternet:
        return L"internet";
    case PhMotwZoneIdRestrictedSites:
        return L"restricted_sites";
    }

    return NULL;
}

VOID AtpAddFileStreams(
    _In_ PVOID Structured,
    _In_ HANDLE FileHandle
    )
{
    PVOID streams;
    PFILE_STREAM_INFORMATION stream;
    PVOID array;

    if (!NT_SUCCESS(PhEnumFileStreams(FileHandle, &streams)))
    {
        AtJsonAddNull(Structured, "streams");
        return;
    }

    array = PhCreateJsonArray();

    for (stream = PH_FIRST_STREAM(streams); stream; stream = PH_NEXT_STREAM(stream))
    {
        PVOID row = PhCreateJsonObject();
        PH_STRINGREF name;

        name.Buffer = stream->StreamName;
        name.Length = stream->StreamNameLength;

        AtJsonAddStringRef(row, "name", &name);
        PhAddJsonObjectUInt64(row, "size", stream->StreamSize.QuadPart);
        PhAddJsonObjectUInt64(row, "allocation_size", stream->StreamAllocationSize.QuadPart);

        // The unnamed data stream is the file itself; everything else is an alternate stream, and
        // that is the distinction worth drawing rather than making the caller parse the name.
        PhAddJsonObjectBoolean(row, "is_alternate", !PhEqualStringRef2(&name, L"::$DATA", TRUE));

        PhAddJsonArrayObject(array, row);
    }

    PhAddJsonObjectValue(Structured, "streams", array);
    PhFree(streams);
}

VOID AtpAddFileHardLinks(
    _In_ PVOID Structured,
    _In_ HANDLE FileHandle
    )
{
    PFILE_LINKS_INFORMATION links;
    PFILE_LINK_ENTRY_INFORMATION link;
    PVOID array;

    if (!NT_SUCCESS(PhEnumFileHardLinks(FileHandle, &links)))
    {
        AtJsonAddNull(Structured, "hard_links");
        return;
    }

    array = PhCreateJsonArray();

    for (link = PH_FIRST_LINK(&links->Entry); link; link = PH_NEXT_LINK(link))
    {
        PVOID row = PhCreateJsonObject();
        PH_STRINGREF name;

        name.Buffer = link->FileName;
        name.Length = link->FileNameLength * sizeof(WCHAR);

        AtJsonAddStringRef(row, "name", &name);
        PhAddJsonObjectUInt64(row, "parent_file_id", link->ParentFileId);
        PhAddJsonArrayObject(array, row);
    }

    PhAddJsonObjectValue(Structured, "hard_links", array);
    PhFree(links);
}

VOID AtpGetFileInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST ULONG attributeFlags[] =
    {
        FILE_ATTRIBUTE_READONLY, FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_SYSTEM,
        FILE_ATTRIBUTE_DIRECTORY, FILE_ATTRIBUTE_ARCHIVE, FILE_ATTRIBUTE_DEVICE,
        FILE_ATTRIBUTE_NORMAL, FILE_ATTRIBUTE_TEMPORARY, FILE_ATTRIBUTE_SPARSE_FILE,
        FILE_ATTRIBUTE_REPARSE_POINT, FILE_ATTRIBUTE_COMPRESSED, FILE_ATTRIBUTE_OFFLINE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, FILE_ATTRIBUTE_ENCRYPTED,
        FILE_ATTRIBUTE_INTEGRITY_STREAM, FILE_ATTRIBUTE_NO_SCRUB_DATA,
        FILE_ATTRIBUTE_PINNED, FILE_ATTRIBUTE_UNPINNED, FILE_ATTRIBUTE_RECALL_ON_OPEN,
        FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS
    };
    static CONST PWSTR attributeNames[] =
    {
        L"readonly", L"hidden", L"system",
        L"directory", L"archive", L"device",
        L"normal", L"temporary", L"sparse_file",
        L"reparse_point", L"compressed", L"offline",
        L"not_content_indexed", L"encrypted",
        L"integrity_stream", L"no_scrub_data",
        L"pinned", L"unpinned", L"recall_on_open",
        L"recall_on_data_access"
    };
    NTSTATUS status;
    PPH_STRING path;
    PPH_STRING nativePath = NULL;
    HANDLE fileHandle;
    PFILE_ALL_INFORMATION allInformation;
    FILE_ID_INFORMATION fileId;
    PH_MOTW_ZONE_ID zoneId;
    PPH_STRING referrerUrl = NULL;
    PPH_STRING hostUrl = NULL;
    USN usn;
    PVOID structured;
    BOOLEAN isDirectory;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")) || path->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        PhClearReference(&path);
        return;
    }

    // FILE_READ_ATTRIBUTES alone, so a file another process holds exclusively still answers, and a
    // directory opens the same way a file does.
    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(path),
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

    if (!NT_SUCCESS(status = PhGetFileAllInformation(fileHandle, &allInformation)))
    {
        AtSetToolStatusError(Result, status, L"Querying the file");
        NtClose(fileHandle);
        PhDereferenceObject(path);
        return;
    }

    isDirectory = !!allInformation->StandardInformation.Directory;

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    PhAddJsonObjectBoolean(structured, "is_directory", isDirectory);
    PhAddJsonObjectUInt64(structured, "size", allInformation->StandardInformation.EndOfFile.QuadPart);
    PhAddJsonObjectUInt64(structured, "allocation_size", allInformation->StandardInformation.AllocationSize.QuadPart);
    PhAddJsonObjectUInt64(structured, "hard_link_count", allInformation->StandardInformation.NumberOfLinks);
    PhAddJsonObjectBoolean(structured, "delete_pending", !!allInformation->StandardInformation.DeletePending);
    AtJsonAddHex(structured, "attributes_value", allInformation->BasicInformation.FileAttributes);
    AtJsonAddFlagStrings(structured, "attributes", allInformation->BasicInformation.FileAttributes,
        attributeFlags, (CONST PWSTR*)attributeNames, RTL_NUMBER_OF(attributeFlags));
    AtJsonAddTime(structured, "creation_time", &allInformation->BasicInformation.CreationTime);
    AtJsonAddTime(structured, "last_access_time", &allInformation->BasicInformation.LastAccessTime);
    AtJsonAddTime(structured, "last_write_time", &allInformation->BasicInformation.LastWriteTime);
    AtJsonAddTime(structured, "change_time", &allInformation->BasicInformation.ChangeTime);
    PhAddJsonObjectUInt64(structured, "index_number", allInformation->InternalInformation.IndexNumber.QuadPart);
    PhAddJsonObjectUInt64(structured, "ea_size", allInformation->EaInformation.EaSize);

    // The file id is 128 bits on ReFS and the low 64 are the NTFS file reference number, so it is
    // reported whole rather than truncated to something that used to be enough.
    if (NT_SUCCESS(PhGetFileId(fileHandle, &fileId)))
    {
        PPH_STRING string = PhBufferToHexStringEx(fileId.FileId.Identifier, sizeof(fileId.FileId.Identifier), FALSE);

        AtJsonAddString(structured, "file_id", string);
        // Hex, like the file id beside it and like every tool that prints a volume serial. It is a
        // 64 bit value and JSON numbers are signed, so a decimal one would clamp.
        AtJsonAddHex(structured, "volume_serial_number", fileId.VolumeSerialNumber);
        PhClearReference(&string);
    }
    else
    {
        AtJsonAddNull(structured, "file_id");
        AtJsonAddNull(structured, "volume_serial_number");
    }

    // The update sequence number changes every time the file does, so two reads that differ mean the
    // file was written between them even if nothing else moved.
    if (NT_SUCCESS(PhGetFileUsn(fileHandle, &usn)))
        PhAddJsonObjectUInt64(structured, "usn", usn);
    else
        AtJsonAddNull(structured, "usn");

    AtpAddFileStreams(structured, fileHandle);

    if (AtJsonGetObjectBoolean(Call->Arguments, "include_hard_links"))
        AtpAddFileHardLinks(structured, fileHandle);
    else
        AtJsonAddNull(structured, "hard_links");

    // PhGetFileMotw opens <name>:Zone.Identifier with PhCreateFile, which takes a native path. A
    // Win32 path here fails to open and every file comes back with no Mark of the Web, which reads
    // as "nothing was downloaded" rather than as a mistake.
    // PhGetFileMotw hands back the values with the line's carriage return still on the end, and a URL
    // with a control character in it is wrong for everything downstream.
    if (nativePath = PhDosPathNameToNtPathName(&path->sr))
    {
        if (NT_SUCCESS(PhGetFileMotw(&nativePath->sr, &zoneId, &referrerUrl, &hostUrl)))
        {
            PVOID motw = PhCreateJsonObject();

            AtJsonAddStringZ(motw, "zone", AtpMotwZoneString(zoneId));
            PhAddJsonObjectUInt64(motw, "zone_id", zoneId);
            AtpTrimTrailingNewline(referrerUrl);
            AtpTrimTrailingNewline(hostUrl);
            AtJsonAddString(motw, "referrer_url", referrerUrl);
            AtJsonAddString(motw, "host_url", hostUrl);
            PhAddJsonObjectValue(structured, "motw", motw);
        }
        else
        {
            AtJsonAddNull(structured, "motw");
        }

        PhClearReference(&referrerUrl);
        PhClearReference(&hostUrl);
        PhDereferenceObject(nativePath);
    }
    else
    {
        AtJsonAddNull(structured, "motw");
    }

    Result->StructuredContent = structured;

    PhFree(allInformation);
    NtClose(fileHandle);
    PhDereferenceObject(path);
}

PONLINECHECKS_INTERFACE AtGetOnlineChecksInterface(
    VOID
    )
{
    static PONLINECHECKS_INTERFACE pluginInterface = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_PLUGIN plugin;

        if (plugin = PhFindPlugin(ONLINECHECKS_PLUGIN_NAME))
        {
            pluginInterface = PhGetPluginInformation(plugin)->Interface;

            if (pluginInterface && pluginInterface->Version < ONLINECHECKS_INTERFACE_VERSION)
                pluginInterface = NULL;
        }

        PhEndInitOnce(&initOnce);
    }

    return pluginInterface;
}

PCWSTR AtpScanLookupString(
    _In_ ONLINECHECKS_LOOKUP_RESULT Lookup
    )
{
    switch (Lookup)
    {
    case OnlineChecksLookupFound:
        return L"found";
    case OnlineChecksLookupNotFound:
        return L"not_cached";
    case OnlineChecksLookupUnavailable:
        return L"unavailable";
    }

    return NULL;
}

// The verdict OnlineChecks already has on disk, and nothing else. No request is made, so a file
// nobody has looked up stays unlooked-up: this answers "has anyone already told us about this",
// which is a different question from "what does VirusTotal say".
VOID AtpGetFileScanResultCached(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PONLINECHECKS_INTERFACE pluginInterface;
    PPH_STRING sha256;
    PPH_STRING path;
    ONLINECHECKS_VIRUSTOTAL_RESULT virusTotal;
    ONLINECHECKS_HYBRIDANALYSIS_RESULT hybridAnalysis;
    ONLINECHECKS_LOOKUP_RESULT lookup;
    LARGE_INTEGER now;
    PVOID structured;
    PVOID entry;

    if (!(pluginInterface = AtGetOnlineChecksInterface()))
    {
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_SUPPORTED,
            L"The OnlineChecks plugin is not loaded, so there is no scan database to read."
            );
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        return;
    }

    sha256 = AtGetArgumentString(Call->Arguments, "sha256");
    path = AtGetArgumentString(Call->Arguments, "path");

    if (!sha256 && path)
        sha256 = AtHashFileSha256(path);

    if (PhIsNullOrEmptyString(sha256))
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            path ? L"The file could not be read to hash it." : L"sha256 or path is required."
            );
        PhClearReference(&sha256);
        PhClearReference(&path);
        return;
    }

    PhQuerySystemTime(&now);

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "sha256", sha256);
    AtJsonAddString(structured, "path", path);

    memset(&virusTotal, 0, sizeof(ONLINECHECKS_VIRUSTOTAL_RESULT));
    lookup = pluginInterface->QueryCachedVirusTotal(sha256, &virusTotal);
    entry = PhCreateJsonObject();
    AtJsonAddStringZ(entry, "lookup", AtpScanLookupString(lookup));

    if (lookup == OnlineChecksLookupFound)
    {
        PhAddJsonObjectUInt64(entry, "http_status", virusTotal.HttpStatus);
        AtJsonAddTime(entry, "expiry", &virusTotal.Expiry);
        PhAddJsonObjectBoolean(entry, "expired", virusTotal.Expiry.QuadPart < now.QuadPart);

        // Only a 200 carried a verdict; the counts on any other status are the zeroes the row was
        // written with, and reporting them as "nothing detected this" would be a lie.
        if (virusTotal.HttpStatus == 200)
        {
            PhAddJsonObjectUInt64(entry, "malicious", virusTotal.Malicious);
            PhAddJsonObjectUInt64(entry, "undetected", virusTotal.Undetected);
        }
        else
        {
            AtJsonAddNull(entry, "malicious");
            AtJsonAddNull(entry, "undetected");
        }
    }

    PhAddJsonObjectValue(structured, "virustotal", entry);

    memset(&hybridAnalysis, 0, sizeof(ONLINECHECKS_HYBRIDANALYSIS_RESULT));
    lookup = pluginInterface->QueryCachedHybridAnalysis(sha256, &hybridAnalysis);
    entry = PhCreateJsonObject();
    AtJsonAddStringZ(entry, "lookup", AtpScanLookupString(lookup));

    if (lookup == OnlineChecksLookupFound)
    {
        PhAddJsonObjectUInt64(entry, "http_status", hybridAnalysis.HttpStatus);
        AtJsonAddTime(entry, "expiry", &hybridAnalysis.Expiry);
        PhAddJsonObjectBoolean(entry, "expired", hybridAnalysis.Expiry.QuadPart < now.QuadPart);

        if (hybridAnalysis.HttpStatus == 200)
        {
            PhAddJsonObjectUInt64(entry, "multiscan_percent", hybridAnalysis.MultiscanResult);
            AtJsonAddString(entry, "family", hybridAnalysis.VxFamily);
        }
        else
        {
            AtJsonAddNull(entry, "multiscan_percent");
            AtJsonAddNull(entry, "family");
        }

        PhClearReference(&hybridAnalysis.VxFamily);
    }

    PhAddJsonObjectValue(structured, "hybrid_analysis", entry);

    Result->StructuredContent = structured;

    PhClearReference(&sha256);
    PhClearReference(&path);
}
