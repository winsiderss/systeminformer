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

PPH_STRING VirusTotalStringToTime(
    _In_ PPH_STRING Time
    )
{
    PPH_STRING result = NULL;
    SYSTEMTIME time = { 0 };
    SYSTEMTIME localTime = { 0 };
    INT count;

    count = swscanf(
        PhGetString(Time),
        L"%hu-%hu-%hu %hu:%hu:%hu",
        &time.wYear,
        &time.wMonth,
        &time.wDay,
        &time.wHour,
        &time.wMinute,
        &time.wSecond
        );

    if (count == 6)
    {
        if (PhSystemTimeToTzSpecificLocalTime(&time, &localTime))
        {
            result = PhFormatDateTime(&localTime);
        }
    }

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

    if (NT_SUCCESS(PhQueryFullAttributesFileWin32(
        Entry->FileName->Buffer,
        &fileAttributeInfo
        )))
    {
        Entry->CreationTime = VirusTotalTimeString(&fileAttributeInfo.CreationTime);
    }

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        Entry->FileName->Buffer,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT // FILE_OPEN_FOR_BACKUP_INTENT
        )))
    {
        if (NT_SUCCESS(HashFileAndResetPosition(
            fileHandle,
            &fileAttributeInfo.EndOfFile,
            Sha256HashAlgorithm,
            &hashString
            )))
        {
            PVOID entry;

            Entry->FileHash = hashString;
            Entry->FileHashAnsi = PhConvertUtf16ToMultiByte(hashString->Buffer);

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
    _In_ PPH_BYTES JsonArray
    )
{
    PPH_BYTES subRequestBuffer = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING urlPathString = NULL;

    if (!PhHttpSocketCreate(&httpContext, L"SystemInformer_3.0_A2D1C96D_D25915D9"))
        goto CleanupExit;

    if (!PhHttpSocketConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT))
        goto CleanupExit;

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        urlPathString->Buffer,
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, JsonArray->Buffer, (ULONG)JsonArray->Length))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(subRequestBuffer = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    PhClearReference(&urlPathString);

    if (JsonArray)
        PhDereferenceObject(JsonArray);

    return subRequestBuffer;
}

PVIRUSTOTAL_FILE_REPORT VirusTotalRequestFileReport(
    _In_ PPH_STRING FileHash
    )
{
    PVIRUSTOTAL_FILE_REPORT result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING urlPathString = NULL;
    PVOID jsonRootObject = NULL;
    PVOID jsonScanObject;

    if (!PhHttpSocketCreate(&httpContext, L"SystemInformer_3.0_A2D1C96D_D25915D9"))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        L"www.virustotal.com",
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        PhGetString(urlPathString),
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

    if (!(jsonRootObject = PhCreateJsonParser(jsonString->Buffer)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_REPORT));
    result->ResponseCode = PhGetJsonValueAsUInt64(jsonRootObject, "response_code");
    result->StatusMessage = PhGetJsonValueAsString(jsonRootObject, "verbose_msg");
    result->PermaLink = PhGetJsonValueAsString(jsonRootObject, "permalink");
    result->ScanDate = PhGetJsonValueAsString(jsonRootObject, "scan_date");
    result->ScanId = PhGetJsonValueAsString(jsonRootObject, "scan_id");
    result->Total = PhFormatUInt64(PhGetJsonValueAsUInt64(jsonRootObject, "total"), FALSE);
    result->Positives = PhFormatUInt64(PhGetJsonValueAsUInt64(jsonRootObject, "positives"), FALSE);
    //result->Md5 = PhGetJsonValueAsString(jsonRootObject, "md5");
    //result->Sha1 = PhGetJsonValueAsString(jsonRootObject, "sha1");
    //result->Sha256 = PhGetJsonValueAsString(jsonRootObject, "sha256");

    if (jsonScanObject = PhGetJsonObject(jsonRootObject, "scans"))
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
                entry->Detected = PhGetJsonObjectBool(object->Entry, "detected");
                entry->EngineVersion = PhGetJsonValueAsString(object->Entry, "version");
                entry->DetectionName = PhGetJsonValueAsString(object->Entry, "result");
                entry->DatabaseDate = PhGetJsonValueAsString(object->Entry, "update");
                PhAddItemList(result->ScanResults, entry);

                PhFree(object);
            }

            PhDereferenceObject(jsonArrayList);
        }
    }

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);

    return result;
}

VOID VirusTotalFreeFileReport(
    _In_ PVIRUSTOTAL_FILE_REPORT FileReport
    )
{
    PhClearReference(&FileReport->StatusMessage);
    PhClearReference(&FileReport->PermaLink);
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
    _In_ PPH_STRING FileHash
    )
{
    PVIRUSTOTAL_API_RESPONSE result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING urlPathString = NULL;
    PVOID jsonRootObject = NULL;

    if (!PhHttpSocketCreate(&httpContext, L"SystemInformer_3.0_A2D1C96D_D25915D9"))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        L"www.virustotal.com",
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        PhGetString(urlPathString),
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

    if (!(jsonRootObject = PhCreateJsonParser(jsonString->Buffer)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_API_RESPONSE));
    result->ResponseCode = PhGetJsonValueAsUInt64(jsonRootObject, "response_code");
    result->StatusMessage = PhGetJsonValueAsString(jsonRootObject, "verbose_msg");
    result->PermaLink = PhGetJsonValueAsString(jsonRootObject, "permalink");
    result->ScanId = PhGetJsonValueAsString(jsonRootObject, "scan_id");

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);

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

PVIRUSTOTAL_API_RESPONSE VirusTotalRequestIpAddressReport(
    _In_ PPH_STRING IpAddress
    )
{
    PVIRUSTOTAL_API_RESPONSE result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING urlPathString = NULL;
    PVOID jsonRootObject = NULL;

    if (!PhHttpSocketCreate(&httpContext, L"SystemInformer_3.0_A2D1C96D_D25915D9"))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        L"www.virustotal.com",
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        PhGetString(urlPathString),
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

    if (!(jsonRootObject = PhCreateJsonParser(jsonString->Buffer)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_API_RESPONSE));
    //result->ResponseCode = PhGetJsonValueAsLong64(jsonRootObject, "response_code");
    //result->StatusMessage = PhGetJsonValueAsString(jsonRootObject, "verbose_msg");
    //result->PermaLink = PhGetJsonValueAsString(jsonRootObject, "permalink");
    //result->ScanId = PhGetJsonValueAsString(jsonRootObject, "scan_id");

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);
    return result;
}
