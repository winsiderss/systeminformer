/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2020
 *     jxy-s   2026
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

BOOLEAN VirusTotalHeaderStrings(
    _Out_ PPH_STRING* String,
    _Out_ PPH_STRING* Value,
    _In_ PPH_STRING ApiKey,
    _In_opt_ PPH_STRING HashString,
    _In_ PCWSTR ServiceName
    )
{
    *Value = PhConcatStrings(2, ServiceName, PhGetStringOrEmpty(HashString));
    *String = PhConcatStrings(2, L"x-apikey: ", PhGetStringOrEmpty(ApiKey));
    return TRUE;
}

PPH_BYTES VirusTotalSendHttpRequest(
    _In_ PPH_BYTES JsonArray,
    _In_ PPH_STRING FilePat
    )
{
    NTSTATUS status;
    PPH_BYTES subRequestBuffer = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpPathString = NULL;
    PPH_STRING httpHeaderString = NULL;

    if (!VirusTotalHeaderStrings(&httpHeaderString, &httpPathString, FilePat, NULL, L"/api/v3/files/"))
        return NULL;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", httpPathString->Buffer, PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"Content-Type: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, JsonArray->Buffer, (ULONG)JsonArray->Length, (ULONG)JsonArray->Length)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &subRequestBuffer)))
        goto CleanupExit;

CleanupExit:
    PhHttpDestroy(httpContext);
    PhClearReference(&httpHeaderString);
    PhClearReference(&httpPathString);

    return subRequestBuffer;
}

NTSTATUS VirusTotalRequestFileReport(
    _In_ PPH_STRING FileHash,
    _In_ PPH_STRING ApiKey,
    _Out_ PVIRUSTOTAL_FILE_REPORT* FileReport
    )
{
    NTSTATUS status;
    PVIRUSTOTAL_FILE_REPORT result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpPathString = NULL;
    PPH_STRING httpHeaderString = NULL;
    PVOID jsonRootObject = NULL;
    ULONG httpStatus = PH_HTTP_STATUS_OK;

    if (!VirusTotalHeaderStrings(&httpHeaderString, &httpPathString, ApiKey, FileHash, L"/api/v3/files/"))
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(httpPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"Content-Type: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_REPORT));
    result->HttpStatus = httpStatus;
    if (httpStatus == 200)
    {
        PVOID dataObject = PhGetJsonObject(jsonRootObject, "data");
        PVOID attributesObject = PhGetJsonObject(dataObject, "attributes");
        PVOID statsObject = PhGetJsonObject(attributesObject, "last_analysis_stats");

        result->ScanDate = PhFormatUInt64(PhGetJsonValueAsUInt64(attributesObject, "last_analysis_date"), FALSE);
        result->ScanId = PhGetJsonValueAsString(dataObject, "id");
        result->Undetected = PhGetJsonValueAsUInt64(statsObject, "undetected");
        result->Malicious = PhGetJsonValueAsUInt64(statsObject, "malicious");
    }

    *FileReport = result;
    result = NULL;

CleanupExit:
    PhHttpDestroy(httpContext);

    PhClearReference(&httpHeaderString);
    PhClearReference(&httpPathString);
    PhClearReference(&jsonString);

    return status;
}

VOID VirusTotalFreeFileReport(
    _In_ PVIRUSTOTAL_FILE_REPORT FileReport
    )
{
    PhClearReference(&FileReport->ScanId);
    PhClearReference(&FileReport->ScanDate);
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
    PPH_STRING httpPathString = NULL;
    PPH_STRING httpHeaderString = NULL;
    PVOID jsonRootObject = NULL;

    if (!VirusTotalHeaderStrings(&httpHeaderString, &httpPathString, FilePat, FileHash, L"/api/v3/files/"))
        return NULL;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", PhGetString(httpPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"Content-Type: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
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

    PhHttpDestroy(httpContext);
    PhClearReference(&jsonRootObject);
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

BOOLEAN HybridAnalysisHeaderStrings(
    _Out_ PPH_STRING* String,
    _Out_ PPH_STRING* Value,
    _In_ PPH_STRING ApiKey,
    _In_opt_ PPH_STRING HashString,
    _In_ PCWSTR ServiceName
    )
{
    *Value = PhConcatStrings(2, ServiceName, PhGetStringOrEmpty(HashString));
    *String = PhConcatStrings(2, L"api-key: ", PhGetStringOrEmpty(ApiKey));
    return TRUE;
}

NTSTATUS HybridAnalysisRequestFileReport(
    _In_ PPH_STRING FileHash,
    _In_ PPH_STRING ApiKey,
    _Out_ PHYBRIDANALYSIS_FILE_REPORT* FileReport
    )
{
    NTSTATUS status;
    PHYBRIDANALYSIS_FILE_REPORT result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpPathString = NULL;
    PPH_STRING httpHeaderString = NULL;
    PVOID jsonRootObject = NULL;
    ULONG httpStatus = PH_HTTP_STATUS_OK;

    if (!HybridAnalysisHeaderStrings(&httpHeaderString, &httpPathString, ApiKey, FileHash, L"/api/v2/overview/"))
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"hybrid-analysis.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(httpPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(HYBRIDANALYSIS_FILE_REPORT));
    result->HttpStatus = httpStatus;
    if (httpStatus == 200)
    {
        result->ThreatScore = PhGetJsonValueAsUInt64(jsonRootObject, "threat_score");
        result->Verdict = PhGetJsonValueAsString(jsonRootObject, "verdict");
        if (PhIsNullOrEmptyString(result->Verdict))
            result->Verdict = PhReferenceEmptyString();
        result->MultiscanResult = PhGetJsonValueAsUInt64(jsonRootObject, "multiscan_result");
        result->VxFamily = PhGetJsonValueAsString(jsonRootObject, "vx_family");;
        if (PhIsNullOrEmptyString(result->VxFamily))
            result->VxFamily = PhReferenceEmptyString();
    }

    *FileReport = result;
    result = NULL;

CleanupExit:
    PhHttpDestroy(httpContext);

    PhClearReference(&httpHeaderString);
    PhClearReference(&httpPathString);
    PhClearReference(&jsonString);

    return status;
}

VOID HybridAnalysisFreeFileReport(
    _In_ PHYBRIDANALYSIS_FILE_REPORT FileReport
    )
{
    PhClearReference(&FileReport->VxFamily);
    PhClearReference(&FileReport->Verdict);
    PhFree(FileReport);
}
