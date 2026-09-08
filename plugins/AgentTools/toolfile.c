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
    NTSTATUS status;
    PVOID streams;
    PFILE_STREAM_INFORMATION stream;
    PVOID array;

    status = PhEnumFileStreams(FileHandle, &streams);

    // A directory has no streams at all. That is an empty list, not an unreadable one, and the two
    // have to look different or "no alternate stream here" and "could not look" read the same.
    if (status == STATUS_NO_MORE_ENTRIES)
    {
        PhAddJsonObjectValue(Structured, "streams", PhCreateJsonArray());
        return;
    }

    if (!NT_SUCCESS(status))
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
    NTSTATUS status;
    PFILE_LINKS_INFORMATION links;
    PFILE_LINK_ENTRY_INFORMATION link;
    PVOID array;

    status = PhEnumFileHardLinks(FileHandle, &links);

    if (status == STATUS_NO_MORE_ENTRIES)
    {
        PhAddJsonObjectValue(Structured, "hard_links", PhCreateJsonArray());
        return;
    }

    if (!NT_SUCCESS(status))
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

VOID AtpAddFileAttributes(
    _In_ PVOID Object,
    _In_ ULONG Attributes
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

    AtJsonAddHex(Object, "attributes_value", Attributes);
    AtJsonAddFlagStrings(Object, "attributes", Attributes,
        attributeFlags, (CONST PWSTR*)attributeNames, RTL_NUMBER_OF(attributeFlags));
}

VOID AtpGetFileInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
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
    AtpAddFileAttributes(structured, allInformation->BasicInformation.FileAttributes);
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

// Asking a third party about a file. Only the hash goes out, never the file, but a hash is enough to
// tell someone that this machine holds this exact file - which is why these sit in the egress tier
// and are asked about every time rather than granted for a session.

// A SHA-256 is 64 hexadecimal characters and nothing else. Checked here rather than sent, because a
// malformed hash is a request that leaves the machine and comes back with nothing.
BOOLEAN AtpIsSha256(
    _In_opt_ PPH_STRING Hash
    )
{
    SIZE_T i;

    if (!Hash || Hash->Length != 64 * sizeof(WCHAR))
        return FALSE;

    for (i = 0; i < 64; i++)
    {
        WCHAR c = Hash->Buffer[i];

        if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')))
            return FALSE;
    }

    return TRUE;
}

PPH_STRING AtpResolveLookupHash(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING sha256;
    PPH_STRING path;

    sha256 = AtGetArgumentString(Call->Arguments, "sha256");
    path = AtGetArgumentString(Call->Arguments, "path");

    if (!sha256 && path)
        sha256 = AtHashFileSha256(path);

    PhClearReference(&path);

    if (!AtpIsSha256(sha256))
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            sha256 ? L"sha256 must be 64 hexadecimal characters." : L"sha256 or a readable path is required."
            );
        PhClearReference(&sha256);
        return NULL;
    }

    return sha256;
}

VOID AtpLookupFileHashVirusTotal(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PONLINECHECKS_INTERFACE pluginInterface;
    PPH_STRING sha256;
    ONLINECHECKS_VIRUSTOTAL_REPORT report;
    ONLINECHECKS_VIRUSTOTAL_RESULT cached;
    PVOID structured;

    if (!(pluginInterface = AtGetOnlineChecksInterface()))
    {
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_SUPPORTED,
            L"The OnlineChecks plugin is not loaded, so there is nothing to ask VirusTotal with."
            );
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        return;
    }

    if (!(sha256 = AtpResolveLookupHash(Call, Result)))
        return;

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "sha256", sha256);

    // A cached verdict answers without a request. The caller can turn that off, because a stale
    // verdict is exactly what someone re-checking a file is trying to get past.
    if (AtJsonGetObjectBoolean(Call->Arguments, "force_refresh") == FALSE)
    {
        LARGE_INTEGER now;

        PhQuerySystemTime(&now);
        memset(&cached, 0, sizeof(ONLINECHECKS_VIRUSTOTAL_RESULT));

        if (pluginInterface->QueryCachedVirusTotal(sha256, &cached) == OnlineChecksLookupFound &&
            cached.Expiry.QuadPart > now.QuadPart)
        {
            PhAddJsonObjectBoolean(structured, "from_cache", TRUE);
            PhAddJsonObjectUInt64(structured, "http_status", cached.HttpStatus);
            AtJsonAddNull(structured, "scan_date");

            if (cached.HttpStatus == 200)
            {
                PhAddJsonObjectUInt64(structured, "malicious", cached.Malicious);
                PhAddJsonObjectUInt64(structured, "undetected", cached.Undetected);
            }
            else
            {
                AtJsonAddNull(structured, "malicious");
                AtJsonAddNull(structured, "undetected");
            }

            Result->StructuredContent = structured;
            PhDereferenceObject(sha256);
            return;
        }
    }

    memset(&report, 0, sizeof(ONLINECHECKS_VIRUSTOTAL_REPORT));
    status = pluginInterface->LookupVirusTotal(sha256, &report);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Asking VirusTotal");
        PhFreeJsonObject(structured);
        PhDereferenceObject(sha256);
        return;
    }

    PhAddJsonObjectBoolean(structured, "from_cache", FALSE);
    PhAddJsonObjectUInt64(structured, "http_status", report.HttpStatus);
    AtJsonAddString(structured, "scan_date", report.ScanDate);

    // 404 is a file VirusTotal has never been given. Reporting zero detections for it would read as
    // a clean verdict, which is the opposite of what it means.
    if (report.HttpStatus == 200)
    {
        PhAddJsonObjectUInt64(structured, "malicious", report.Malicious);
        PhAddJsonObjectUInt64(structured, "undetected", report.Undetected);
    }
    else
    {
        AtJsonAddNull(structured, "malicious");
        AtJsonAddNull(structured, "undetected");
    }

    Result->StructuredContent = structured;

    PhClearReference(&report.ScanDate);
    PhDereferenceObject(sha256);
}

VOID AtpLookupFileHashHybridAnalysis(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PONLINECHECKS_INTERFACE pluginInterface;
    PPH_STRING sha256;
    ONLINECHECKS_HYBRIDANALYSIS_REPORT report;
    ONLINECHECKS_HYBRIDANALYSIS_RESULT cached;
    PVOID structured;

    if (!(pluginInterface = AtGetOnlineChecksInterface()))
    {
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_SUPPORTED,
            L"The OnlineChecks plugin is not loaded, so there is nothing to ask Hybrid Analysis with."
            );
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        return;
    }

    if (!(sha256 = AtpResolveLookupHash(Call, Result)))
        return;

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "sha256", sha256);

    if (AtJsonGetObjectBoolean(Call->Arguments, "force_refresh") == FALSE)
    {
        LARGE_INTEGER now;

        PhQuerySystemTime(&now);
        memset(&cached, 0, sizeof(ONLINECHECKS_HYBRIDANALYSIS_RESULT));

        if (pluginInterface->QueryCachedHybridAnalysis(sha256, &cached) == OnlineChecksLookupFound &&
            cached.Expiry.QuadPart > now.QuadPart)
        {
            PhAddJsonObjectBoolean(structured, "from_cache", TRUE);
            PhAddJsonObjectUInt64(structured, "http_status", cached.HttpStatus);
            AtJsonAddNull(structured, "threat_score");
            AtJsonAddNull(structured, "verdict");

            if (cached.HttpStatus == 200)
            {
                PhAddJsonObjectUInt64(structured, "multiscan_percent", cached.MultiscanResult);
                AtJsonAddString(structured, "family", cached.VxFamily);
            }
            else
            {
                AtJsonAddNull(structured, "multiscan_percent");
                AtJsonAddNull(structured, "family");
            }

            Result->StructuredContent = structured;
            PhClearReference(&cached.VxFamily);
            PhDereferenceObject(sha256);
            return;
        }

        PhClearReference(&cached.VxFamily);
    }

    memset(&report, 0, sizeof(ONLINECHECKS_HYBRIDANALYSIS_REPORT));
    status = pluginInterface->LookupHybridAnalysis(sha256, &report);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Asking Hybrid Analysis");
        PhFreeJsonObject(structured);
        PhDereferenceObject(sha256);
        return;
    }

    PhAddJsonObjectBoolean(structured, "from_cache", FALSE);
    PhAddJsonObjectUInt64(structured, "http_status", report.HttpStatus);

    if (report.HttpStatus == 200)
    {
        PhAddJsonObjectUInt64(structured, "multiscan_percent", report.MultiscanResult);
        PhAddJsonObjectUInt64(structured, "threat_score", report.ThreatScore);
        AtJsonAddString(structured, "verdict", report.Verdict);
        AtJsonAddString(structured, "family", report.VxFamily);
    }
    else
    {
        AtJsonAddNull(structured, "multiscan_percent");
        AtJsonAddNull(structured, "threat_score");
        AtJsonAddNull(structured, "verdict");
        AtJsonAddNull(structured, "family");
    }

    Result->StructuredContent = structured;

    PhClearReference(&report.Verdict);
    PhClearReference(&report.VxFamily);
    PhDereferenceObject(sha256);
}

// A registry key: what is under it and what the values say. Nothing else in the server can read the
// registry, and persistence lives there - a Run entry, a service's ImagePath, a shell extension.

// Binary values can be large and are rarely interesting past the first part of them.
#define AT_REGISTRY_MAX_DATA 4096
#define AT_REGISTRY_MAX_SUBKEYS 4096

typedef struct _AT_REGISTRY_CONTEXT
{
    AT_ROWS Values;
    PPH_LIST Subkeys;
    PPH_STRING NameContains;
    ULONG MaxData;
} AT_REGISTRY_CONTEXT, *PAT_REGISTRY_CONTEXT;

PCWSTR AtpRegistryTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case REG_NONE:
        return L"REG_NONE";
    case REG_SZ:
        return L"REG_SZ";
    case REG_EXPAND_SZ:
        return L"REG_EXPAND_SZ";
    case REG_BINARY:
        return L"REG_BINARY";
    case REG_DWORD:
        return L"REG_DWORD";
    case REG_DWORD_BIG_ENDIAN:
        return L"REG_DWORD_BIG_ENDIAN";
    case REG_LINK:
        return L"REG_LINK";
    case REG_MULTI_SZ:
        return L"REG_MULTI_SZ";
    case REG_RESOURCE_LIST:
        return L"REG_RESOURCE_LIST";
    case REG_FULL_RESOURCE_DESCRIPTOR:
        return L"REG_FULL_RESOURCE_DESCRIPTOR";
    case REG_RESOURCE_REQUIREMENTS_LIST:
        return L"REG_RESOURCE_REQUIREMENTS_LIST";
    case REG_QWORD:
        return L"REG_QWORD";
    }

    return NULL;
}

// Registry string data carries whatever the writer put there: it is not guaranteed to be terminated,
// and not guaranteed to be an even number of bytes either.
PPH_STRING AtpRegistryString(
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ ULONG DataLength
    )
{
    SIZE_T length = DataLength & ~(SIZE_T)(sizeof(WCHAR) - 1);

    if (length >= sizeof(WCHAR) &&
        *(PWCHAR)PTR_ADD_OFFSET(Data, length - sizeof(WCHAR)) == UNICODE_NULL)
    {
        length -= sizeof(WCHAR);
    }

    return PhCreateStringEx(Data, length);
}

VOID AtpAddRegistryValue(
    _In_ PVOID Row,
    _In_ ULONG Type,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _In_ ULONG MaxData
    )
{
    PhAddJsonObjectUInt64(Row, "data_size", DataLength);
    AtJsonAddStringZ(Row, "type", AtpRegistryTypeString(Type));
    PhAddJsonObjectUInt64(Row, "type_value", Type);
    PhAddJsonObjectBoolean(Row, "truncated", DataLength > MaxData);

    switch (Type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_LINK:
        {
            PPH_STRING string = AtpRegistryString(Data, min(DataLength, MaxData));

            AtJsonAddString(Row, "data", string);
            AtJsonAddNull(Row, "data_strings");
            PhClearReference(&string);
        }
        break;
    case REG_MULTI_SZ:
        {
            // A sequence of terminated strings ending in an empty one. A writer that leaves the
            // final terminator off is common enough that running off the end has to be guarded.
            PVOID array = PhCreateJsonArray();
            SIZE_T length = min(DataLength, MaxData) & ~(SIZE_T)(sizeof(WCHAR) - 1);
            SIZE_T offset = 0;

            while (offset < length)
            {
                PWCHAR start = PTR_ADD_OFFSET(Data, offset);
                SIZE_T remaining = (length - offset) / sizeof(WCHAR);
                SIZE_T i;

                for (i = 0; i < remaining && start[i] != UNICODE_NULL; i++)
                    NOTHING;

                if (i == 0)
                    break;

                PhAddJsonArrayObject(array, PhCreateJsonStringObject(
                    PH_AUTO_T(PH_BYTES, PhConvertUtf16ToUtf8Ex(start, i * sizeof(WCHAR)))->Buffer));

                offset += (i + 1) * sizeof(WCHAR);
            }

            AtJsonAddNull(Row, "data");
            PhAddJsonObjectValue(Row, "data_strings", array);
        }
        break;
    case REG_DWORD:
        if (DataLength >= sizeof(ULONG))
            PhAddJsonObjectUInt64(Row, "data_number", *(PULONG)Data);
        AtJsonAddNull(Row, "data");
        AtJsonAddNull(Row, "data_strings");
        break;
    case REG_DWORD_BIG_ENDIAN:
        if (DataLength >= sizeof(ULONG))
            PhAddJsonObjectUInt64(Row, "data_number", _byteswap_ulong(*(PULONG)Data));
        AtJsonAddNull(Row, "data");
        AtJsonAddNull(Row, "data_strings");
        break;
    case REG_QWORD:
        if (DataLength >= sizeof(ULONG64))
            PhAddJsonObjectUInt64(Row, "data_number", *(PULONG64)Data);
        AtJsonAddNull(Row, "data");
        AtJsonAddNull(Row, "data_strings");
        break;
    default:
        {
            // Everything else is bytes, and hex is the only honest rendering of bytes.
            PPH_STRING string = PhBufferToHexStringEx(Data, min(DataLength, MaxData), FALSE);

            AtJsonAddString(Row, "data_hex", string);
            AtJsonAddNull(Row, "data");
            AtJsonAddNull(Row, "data_strings");
            PhClearReference(&string);
        }
        break;
    }
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpRegistryValueCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PAT_REGISTRY_CONTEXT context = Context;
    PKEY_VALUE_FULL_INFORMATION information = Information;
    PPH_STRING name;
    PVOID row;

    if (!context)
        return FALSE;

    // The unnamed value is the key's default, which every editor shows as "(Default)".
    name = information->NameLength ?
        PhCreateStringEx(information->Name, information->NameLength) : NULL;

    if (context->NameContains && !AtContainsString(name, context->NameContains))
    {
        PhClearReference(&name);
        return TRUE;
    }

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", name);
    PhAddJsonObjectBoolean(row, "is_default", !information->NameLength);
    AtpAddRegistryValue(
        row,
        information->Type,
        PTR_ADD_OFFSET(information, information->DataOffset),
        information->DataLength,
        context->MaxData
        );
    AtAddRow(&context->Values, row);

    PhClearReference(&name);

    return TRUE;
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpRegistrySubkeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PAT_REGISTRY_CONTEXT context = Context;
    PKEY_BASIC_INFORMATION information = Information;
    PVOID row;
    PPH_STRING name;

    if (!context || context->Subkeys->Count >= AT_REGISTRY_MAX_SUBKEYS)
        return FALSE;

    name = PhCreateStringEx(information->Name, information->NameLength);

    if (context->NameContains && !AtContainsString(name, context->NameContains))
    {
        PhClearReference(&name);
        return TRUE;
    }

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", name);
    AtJsonAddTime(row, "last_write_time", &information->LastWriteTime);
    PhAddItemList(context->Subkeys, row);

    PhClearReference(&name);

    return TRUE;
}

VOID AtpReadRegistryKey(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_REGISTRY_CONTEXT context;
    PPH_STRING path;
    PPH_STRING subKey = NULL;
    HANDLE rootDirectory = NULL;
    HANDLE keyHandle;
    PCWSTR nativeRoot = NULL;
    KEY_FULL_INFORMATION keyInformation;
    LARGE_INTEGER lastWriteTime;
    ULONG64 maxData = AT_REGISTRY_MAX_DATA;
    PVOID structured;
    PVOID array;
    ULONG i;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")) || path->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        PhClearReference(&path);
        return;
    }

    memset(&context, 0, sizeof(AT_REGISTRY_CONTEXT));
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.MaxData = AT_REGISTRY_MAX_DATA;

    if (AtGetArgumentUInt64(Call->Arguments, "max_data_bytes", &maxData))
        context.MaxData = (ULONG)min(max(maxData, 16), 1024 * 1024);

    // A hive prefix picks the root and the rest is relative to it; a native path opens on its own.
    AtParseRegistryPath(path, &rootDirectory, &subKey, &nativeRoot);

    status = PhOpenKey(&keyHandle, KEY_READ, rootDirectory, &subKey->sr, 0);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Opening the key");
        PhDereferenceObject(subKey);
        PhClearReference(&context.NameContains);
        PhDereferenceObject(path);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    AtJsonAddStringZ(structured, "root", nativeRoot);

    if (NT_SUCCESS(PhQueryKeyLastWriteTime(keyHandle, &lastWriteTime)))
        AtJsonAddTime(structured, "last_write_time", &lastWriteTime);
    else
        AtJsonAddNull(structured, "last_write_time");

    if (NT_SUCCESS(PhQueryKeyInformation(keyHandle, &keyInformation)))
    {
        PhAddJsonObjectUInt64(structured, "subkey_count", keyInformation.SubKeys);
        PhAddJsonObjectUInt64(structured, "value_count", keyInformation.Values);
    }
    else
    {
        AtJsonAddNull(structured, "subkey_count");
        AtJsonAddNull(structured, "value_count");
    }

    AtInitializeRows(&context.Values, Call->Arguments);
    context.Subkeys = PhCreateList(16);

    if (AtJsonGetObjectMember(Call->Arguments, "include_values", PH_JSON_OBJECT_TYPE_BOOLEAN) == NULL ||
        AtJsonGetObjectBoolean(Call->Arguments, "include_values"))
    {
        PhEnumerateValueKey(keyHandle, KeyValueFullInformation, AtpRegistryValueCallback, &context);
    }

    if (AtJsonGetObjectMember(Call->Arguments, "include_subkeys", PH_JSON_OBJECT_TYPE_BOOLEAN) == NULL ||
        AtJsonGetObjectBoolean(Call->Arguments, "include_subkeys"))
    {
        PhEnumerateKey(keyHandle, KeyBasicInformation, AtpRegistrySubkeyCallback, &context);
    }

    AtAddRows(structured, "values", &context.Values);

    array = PhCreateJsonArray();

    for (i = 0; i < context.Subkeys->Count; i++)
        PhAddJsonArrayObject(array, context.Subkeys->Items[i]);

    PhAddJsonObjectValue(structured, "subkeys", array);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Values);
    PhDereferenceObject(context.Subkeys);
    NtClose(keyHandle);
    PhDereferenceObject(subKey);
    PhClearReference(&context.NameContains);
    PhDereferenceObject(path);
}

// Filesystem context: what is next to a file, and when. A listing is capped and never recursive -
// the tool answers "what is in this directory", and a caller that wants a tree asks for each one.
typedef struct _AT_DIRECTORY_CONTEXT
{
    PAT_ROWS Rows;
    PPH_STRING NameContains;
    BOOLEAN DirectoriesOnly;
    BOOLEAN FilesOnly;
    ULONG EnumeratedCount;
} AT_DIRECTORY_CONTEXT, *PAT_DIRECTORY_CONTEXT;

_Function_class_(PH_ENUM_DIRECTORY_FILE)
BOOLEAN NTAPI AtpDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PAT_DIRECTORY_CONTEXT context = Context;
    PH_STRINGREF name;
    PPH_STRING nameString;
    PVOID row;
    BOOLEAN isDirectory;

    if (!context)
        return FALSE;

    name.Buffer = Information->FileName;
    name.Length = Information->FileNameLength;

    // The two names every directory has for itself are not entries in it.
    if (PhEqualStringRef2(&name, L".", FALSE) || PhEqualStringRef2(&name, L"..", FALSE))
        return TRUE;

    isDirectory = !!(Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY);

    if ((context->DirectoriesOnly && !isDirectory) || (context->FilesOnly && isDirectory))
        return TRUE;

    nameString = PhCreateString2(&name);

    if (!AtContainsString(nameString, context->NameContains))
    {
        PhDereferenceObject(nameString);
        return TRUE;
    }

    context->EnumeratedCount++;

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", nameString);
    PhAddJsonObjectBoolean(row, "is_directory", isDirectory);

    // A directory's size is whatever the filesystem keeps for it, not the size of what is in it.
    PhAddJsonObjectUInt64(row, "size", Information->EndOfFile.QuadPart);
    PhAddJsonObjectUInt64(row, "allocation_size", Information->AllocationSize.QuadPart);
    AtpAddFileAttributes(row, Information->FileAttributes);
    AtJsonAddTime(row, "creation_time", &Information->CreationTime);
    AtJsonAddTime(row, "last_access_time", &Information->LastAccessTime);
    AtJsonAddTime(row, "last_write_time", &Information->LastWriteTime);
    AtJsonAddTime(row, "change_time", &Information->ChangeTime);

    AtAddRow(context->Rows, row);
    PhDereferenceObject(nameString);

    return TRUE;
}

VOID AtpListDirectory(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_DIRECTORY_CONTEXT context;
    AT_ROWS rows;
    PPH_STRING path;
    PPH_STRING pattern;
    PH_STRINGREF patternRef;
    HANDLE directoryHandle;
    PVOID structured;

    if (!(path = AtGetArgumentString(Call->Arguments, "path")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required.");
        return;
    }

    memset(&context, 0, sizeof(AT_DIRECTORY_CONTEXT));
    context.Rows = &rows;
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.DirectoriesOnly = AtJsonGetObjectBoolean(Call->Arguments, "directories_only");
    context.FilesOnly = AtJsonGetObjectBoolean(Call->Arguments, "files_only");
    pattern = AtGetArgumentString(Call->Arguments, "pattern");

    if (context.DirectoriesOnly && context.FilesOnly)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
            L"directories_only and files_only cannot both be set.");
        goto CleanupExit;
    }

    // FILE_LIST_DIRECTORY is the only right this needs; a directory nobody may list refuses here
    // rather than coming back empty.
    status = PhCreateFileWin32(
        &directoryHandle,
        PhGetString(path),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Opening the directory");
        goto CleanupExit;
    }

    AtInitializeRows(&rows, Call->Arguments);

    if (pattern)
        patternRef = pattern->sr;

    status = PhEnumDirectoryFile(directoryHandle, pattern ? &patternRef : NULL, AtpDirectoryCallback, &context);
    NtClose(directoryHandle);

    // Nothing matched is not a failure; the pattern being nonsense is.
    if (!NT_SUCCESS(status) && status != STATUS_NO_MORE_FILES && status != STATUS_NO_SUCH_FILE)
    {
        AtSetToolStatusError(Result, status, L"Listing the directory");
        AtDeleteRows(&rows);
        goto CleanupExit;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "path", path);
    AtAddRows(structured, "entries", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

CleanupExit:
    PhClearReference(&pattern);
    PhClearReference(&context.NameContains);
    PhDereferenceObject(path);
}
