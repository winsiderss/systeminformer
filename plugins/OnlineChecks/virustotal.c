/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2020
 *
 */

#include "onlnchk.h"

PPH_BYTES VirusTotalTimeString(
    _In_ PLARGE_INTEGER LargeInteger
    )
{
    PH_STRING_BUILDER stringBuilder;
    SYSTEMTIME systemTime;
    PPH_STRING dateString;
    PPH_STRING timeString;
    PPH_STRING finalString;
    PPH_BYTES string;

    PhInitializeStringBuilder(&stringBuilder, 40);
    PhLargeIntegerToLocalSystemTime(&systemTime, LargeInteger);

    dateString = PhFormatDate(&systemTime, L"yyyy-MM-dd");
    PhAppendStringBuilder(&stringBuilder, &dateString->sr);
    PhDereferenceObject(dateString);

    PhAppendCharStringBuilder(&stringBuilder, L' ');

    timeString = PhFormatTime(&systemTime, L"HH:mm:ss");
    PhAppendStringBuilder(&stringBuilder, &timeString->sr);
    PhDereferenceObject(timeString);

    finalString = PhFinalStringBuilderString(&stringBuilder);
    string = PhConvertUtf16ToUtf8Ex(finalString->Buffer, finalString->Length);
    PhDeleteStringBuilder(&stringBuilder);

    return string;
}

PPH_STRING VirusTotalRelativeTimeString(
    _In_ PLARGE_INTEGER Time
    )
{
    LARGE_INTEGER time;
    LARGE_INTEGER currentTime;
    SYSTEMTIME timeFields;
    PPH_STRING timeRelativeString;
    PPH_STRING timeString;

    time = *Time;
    PhQuerySystemTime(&currentTime);
    timeRelativeString = PH_AUTO(PhFormatTimeSpanRelative(currentTime.QuadPart - time.QuadPart));

    PhLargeIntegerToLocalSystemTime(&timeFields, &time);
    timeString = PhaFormatDateTime(&timeFields);

    return PhFormatString(L"%s ago (%s)", timeRelativeString->Buffer, timeString->Buffer);
}

PPH_STRING VirusTotalDateToTime(
    _In_ ULONG TimeDateStamp
    )
{
    PPH_STRING result;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;

    PhSecondsSince1970ToTime(TimeDateStamp, &time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);
    result = VirusTotalRelativeTimeString(&time);
    //result = PhFormatDateTime(&systemTime);

    return result;
}

VOID VirusTotalBuildJsonArray(
    _In_ PVIRUSTOTAL_FILE_HASH_ENTRY Entry,
    _In_ PVOID JsonArray
    )
{
    HANDLE fileHandle;
    FILE_NETWORK_OPEN_INFORMATION fileAttributeInfo;
    PPH_STRING hashString = NULL;

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        &Entry->FileName->sr,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT // FILE_OPEN_FOR_BACKUP_INTENT
        )))
    {
        if (NT_SUCCESS(PhGetFileFullAttributesInformation(
            fileHandle,
            &fileAttributeInfo
            )))
        {
            Entry->CreationTime = VirusTotalTimeString(&fileAttributeInfo.CreationTime);
        }

        if (NT_SUCCESS(HashFileAndResetPosition(
            fileHandle,
            &fileAttributeInfo.EndOfFile,
            Sha256HashAlgorithm,
            &hashString
            )))
        {
            PVOID entry;

            Entry->FileHash = hashString;
            Entry->FileHashAnsi = PhConvertUtf16ToUtf8(hashString->Buffer);

            entry = PhCreateJsonObject();
            PhAddJsonObject(entry, "autostart_location", "");
            PhAddJsonObject(entry, "autostart_entry", "");
            PhAddJsonObject(entry, "hash", Entry->FileHashAnsi->Buffer);
            PhAddJsonObject(entry, "image_path", Entry->FileNameAnsi->Buffer);
            PhAddJsonObject(entry, "creation_datetime", Entry->CreationTime ? Entry->CreationTime->Buffer : "");
            PhAddJsonArrayObject(JsonArray, entry);
        }

        NtClose(fileHandle);
    }
}

PPH_BYTES VirusTotalSendHttpRequest(
    _In_ PPH_BYTES JsonArray,
    _In_ PPH_STRING FilePat
    )
{
    NTSTATUS status;
    PPH_BYTES subRequestBuffer = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING urlPathString = NULL;
    PPH_STRING urlHeaderString = NULL;

    urlPathString = PhConcatStrings2(L"...", L"...");
    urlHeaderString = PhConcatStrings2(L"x-\x0061\x0070\x0069\x006B\x0065\x0079: ", PhGetStringOrEmpty(FilePat));

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", urlPathString->Buffer, PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &urlHeaderString->sr)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"Content-Type: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, JsonArray->Buffer, (ULONG)JsonArray->Length, (ULONG)JsonArray->Length)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &subRequestBuffer)))
        goto CleanupExit;

CleanupExit:

    if (httpContext)
    {
        PhHttpDestroy(httpContext);
    }

    if (urlHeaderString)
    {
        PhDereferenceObject(urlHeaderString);
    }

    if (urlPathString)
    {
        PhDereferenceObject(urlPathString);
    }

    if (JsonArray)
    {
        PhDereferenceObject(JsonArray);
    }

    return subRequestBuffer;
}

PVIRUSTOTAL_FILE_REPORT VirusTotalRequestFileReport(
    _In_ PPH_STRING FileHash,
    _In_ PPH_STRING FilePat
    )
{
    NTSTATUS status;
    PVIRUSTOTAL_FILE_REPORT result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING urlPathString = NULL;
    PPH_STRING urlHeaderString = NULL;
    PVOID jsonRootObject = NULL;
    PVOID jsonScanObject;

    urlPathString = PhConcatStrings2(L"/api/v3/files/", PhGetStringOrEmpty(FileHash));
    urlHeaderString = PhConcatStrings2(L"x-\x0061\x0070\x0069\x006B\x0065\x0079: ", PhGetStringOrEmpty(FilePat));

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(urlPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &urlHeaderString->sr)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"Content-Type: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, NULL, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
        goto CleanupExit;

    PVOID dataObject = PhGetJsonObject(jsonRootObject, "data");
    PVOID attributesObject = PhGetJsonObject(dataObject, "attributes");
    PVOID statsObject = PhGetJsonObject(attributesObject, "last_analysis_stats");

    result = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_REPORT));
    result->ScanDate = PhFormatUInt64(PhGetJsonValueAsUInt64(attributesObject, "last_analysis_date"), FALSE);
    result->ScanId = PhGetJsonValueAsString(dataObject, "id");
    result->Total = PhFormatUInt64(PhGetJsonValueAsUInt64(statsObject, "undetected"), FALSE);
    result->Positives = PhFormatUInt64(PhGetJsonValueAsUInt64(statsObject, "malicious"), FALSE);

    if (jsonScanObject = PhGetJsonObject(attributesObject, "last_analysis_results"))
    {
        PPH_LIST jsonArrayList;

        if (jsonArrayList = PhGetJsonObjectAsArrayList(jsonScanObject))
        {
            result->ScanResults = PhCreateList(jsonArrayList->Count);

            for (ULONG i = 0; i < jsonArrayList->Count; i++)
            {
                PVIRUSTOTAL_FILE_REPORT_RESULT entry;
                PJSON_ARRAY_LIST_OBJECT object = jsonArrayList->Items[i];

                entry = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_REPORT_RESULT));
                entry->Vendor = PhConvertUtf8ToUtf16(object->Key);
                //entry->Detected = PhGetJsonObjectBool(object->Entry, "detected");
                entry->EngineVersion = PhGetJsonValueAsString(object->Entry, "engine_version");
                entry->DetectionName = PhGetJsonValueAsString(object->Entry, "result");
                entry->DatabaseDate = PhGetJsonValueAsString(object->Entry, "engine_update");
                PhAddItemList(result->ScanResults, entry);

                PhFree(object);
            }

            PhDereferenceObject(jsonArrayList);
        }
    }

CleanupExit:

    if (httpContext)
    {
        PhHttpDestroy(httpContext);
    }

    if (jsonRootObject)
    {
        PhFreeJsonObject(jsonRootObject);
    }

    if (urlHeaderString)
    {
        PhDereferenceObject(urlHeaderString);
    }

    if (urlPathString)
    {
        PhDereferenceObject(urlPathString);
    }

    if (jsonString)
    {
        PhDereferenceObject(jsonString);
    }

    return result;
}

VOID VirusTotalFreeFileReport(
    _In_ PVIRUSTOTAL_FILE_REPORT FileReport
    )
{
    PhClearReference(&FileReport->ScanId);
    PhClearReference(&FileReport->ScanDate);
    PhClearReference(&FileReport->Positives);
    PhClearReference(&FileReport->Total);

    if (FileReport->ScanResults)
    {
        for (ULONG i = 0; i < FileReport->ScanResults->Count; i++)
        {
            PVIRUSTOTAL_FILE_REPORT_RESULT object = FileReport->ScanResults->Items[i];

            PhClearReference(&object->Vendor);
            PhClearReference(&object->EngineVersion);
            PhClearReference(&object->DetectionName);
            PhClearReference(&object->DatabaseDate);

            PhFree(object);
        }

        PhClearReference(&FileReport->ScanResults);
    }

    PhFree(FileReport);
}

PVIRUSTOTAL_API_RESPONSE VirusTotalRequestFileReScan(
    _In_ PPH_STRING FileHash,
    _In_ PPH_STRING FilePat
    )
{
    NTSTATUS status;
    PVIRUSTOTAL_API_RESPONSE result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING urlPathString = NULL;
    PPH_STRING urlHeaderString = NULL;
    PVOID jsonRootObject = NULL;

    urlPathString = PhConcatStrings2(L"/api/v3/.../", PhGetStringOrEmpty(FileHash));
    urlHeaderString = PhConcatStrings2(L"x-\x0061\x0070\x0069\x006B\x0065\x0079: ", PhGetStringOrEmpty(FilePat));

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", PhGetString(urlPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &urlHeaderString->sr)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"Content-Type: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, NULL, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_API_RESPONSE));
    result->ResponseCode = PhGetJsonValueAsUInt64(jsonRootObject, "response_code");
    result->StatusMessage = PhGetJsonValueAsString(jsonRootObject, "verbose_msg");
    result->PermaLink = PhGetJsonValueAsString(jsonRootObject, "permalink");
    result->ScanId = PhGetJsonValueAsString(jsonRootObject, "scan_id");

CleanupExit:

    if (httpContext)
    {
        PhHttpDestroy(httpContext);
    }

    if (jsonRootObject)
    {
        PhFreeJsonObject(jsonRootObject);
    }

    if (urlHeaderString)
    {
        PhDereferenceObject(urlHeaderString);
    }

    if (urlPathString)
    {
        PhDereferenceObject(urlPathString);
    }

    if (jsonString)
    {
        PhDereferenceObject(jsonString);
    }

    return result;
}

VOID VirusTotalFreeFileReScan(
    _In_ PVIRUSTOTAL_API_RESPONSE FileReScan
    )
{
    PhClearReference(&FileReScan->StatusMessage);
    PhClearReference(&FileReScan->PermaLink);
    PhClearReference(&FileReScan->ScanId);

    PhFree(FileReScan);
}
