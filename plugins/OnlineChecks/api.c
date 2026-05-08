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

PPH_STRING ClientIdHeaderString(
    VOID
    )
{
    static const PH_STRINGREF clientIdHeader = PH_STRINGREF_INIT(L"SystemInformer-Client-Id: ");
    PPH_STRING clientId = PhGetStringSetting(SETTING_CLIENT_ID);
    PhMoveReference(&clientId, PhConcatStringRef2(&clientIdHeader, &clientId->sr));
    return clientId;
}

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
    _In_opt_ PPH_STRING ApiKey,
    _In_opt_ PPH_STRING HashString,
    _In_ PCWSTR ServiceName
    )
{
    *Value = PhConcatStrings(2, ServiceName, PhGetStringOrEmpty(HashString));
    if (PhIsNullOrEmptyString(ApiKey))
        *String = PhReferenceEmptyString();
    else
        *String = PhConcatStrings(2, L"x-apikey: ", PhGetString(ApiKey));
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
    _In_opt_ PPH_STRING ApiKey,
    _Out_ PVIRUSTOTAL_FILE_REPORT* FileReport
    )
{
    NTSTATUS status;
    PVIRUSTOTAL_FILE_REPORT result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpPathString = NULL;
    PPH_STRING httpHeaderString = NULL;
    PPH_STRING httpHeaderClientId = NULL;
    PVOID jsonRootObject = NULL;
    ULONG httpStatus = PH_HTTP_STATUS_OK;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(ApiKey))
    {
        if (!VirusTotalHeaderStrings(&httpHeaderString, &httpPathString, NULL, FileHash, L"/onlinechecks/virustotal/api/v3/files/"))
            return STATUS_UNSUCCESSFUL;
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"systeminformer.io", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;

        httpHeaderClientId = ClientIdHeaderString();
    }
    else
    {
        if (!VirusTotalHeaderStrings(&httpHeaderString, &httpPathString, ApiKey, FileHash, L"/api/v3/files/"))
            return STATUS_UNSUCCESSFUL;
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(httpPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;

    if (!PhIsNullOrEmptyString(httpHeaderString))
    {
        if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
            goto CleanupExit;
    }

    if (!PhIsNullOrEmptyString(httpHeaderClientId))
    {
        if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderClientId->sr)))
            goto CleanupExit;
    }

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

    result = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_REPORT));
    result->HttpStatus = httpStatus;
    if (httpStatus == 200)
    {
        if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
            goto CleanupExit;

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

    PhClearReference(&httpHeaderClientId);
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
    _In_opt_ PPH_STRING ApiKey,
    _In_opt_ PPH_STRING HashString,
    _In_ PCWSTR ServiceName
    )
{
    *Value = PhConcatStrings(2, ServiceName, PhGetStringOrEmpty(HashString));
    if (PhIsNullOrEmptyString(ApiKey))
        *String = PhReferenceEmptyString();
    else
        *String = PhConcatStrings(2, L"api-key: ", PhGetString(ApiKey));
    return TRUE;
}

NTSTATUS HybridAnalysisRequestFileReport(
    _In_ PPH_STRING FileHash,
    _In_opt_ PPH_STRING ApiKey,
    _Out_ PHYBRIDANALYSIS_FILE_REPORT* FileReport
    )
{
    NTSTATUS status;
    PHYBRIDANALYSIS_FILE_REPORT result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpPathString = NULL;
    PPH_STRING httpHeaderString = NULL;
    PPH_STRING httpHeaderClientId = NULL;
    PVOID jsonRootObject = NULL;
    ULONG httpStatus = PH_HTTP_STATUS_OK;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(ApiKey))
    {
        if (!HybridAnalysisHeaderStrings(&httpHeaderString, &httpPathString, NULL, FileHash, L"/onlinechecks/hybrid-analysis/api/v2/overview/"))
            return STATUS_UNSUCCESSFUL;
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"systeminformer.io", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;

        httpHeaderClientId = ClientIdHeaderString();
    }
    else
    {
        if (!HybridAnalysisHeaderStrings(&httpHeaderString, &httpPathString, ApiKey, FileHash, L"/api/v2/overview/"))
            return STATUS_UNSUCCESSFUL;
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"hybrid-analysis.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(httpPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;

    if (!PhIsNullOrEmptyString(httpHeaderString))
    {
        if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
            goto CleanupExit;
    }

    if (!PhIsNullOrEmptyString(httpHeaderClientId))
    {
        if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderClientId->sr)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(HYBRIDANALYSIS_FILE_REPORT));
    result->HttpStatus = httpStatus;

    if (httpStatus == 200)
    {
        if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
            goto CleanupExit;

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

    PhClearReference(&httpHeaderClientId);
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

NTSTATUS HybridAnalysisSubmitFile(
    _In_ PPH_STRING FileName,
    _In_opt_ PPH_STRING ApiKey,
    _Out_ PPH_STRING* Id,
    _Out_ PBOOLEAN Finished
    )
{
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER fileSize;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING postBoundary = NULL;
    PPH_STRING baseFileName = NULL;
    PH_STRING_BUILDER httpRequestHeaders;
    PH_STRING_BUILDER httpPostHeader;
    PH_STRING_BUILDER httpPostFooter;
    BOOLEAN buildersInitialized = FALSE;
    PPH_BYTES asciiPostData = NULL;
    PPH_BYTES asciiFooterData = NULL;
    PPH_BYTES jsonString = NULL;
    PVOID jsonRootObject = NULL;
    ULONG totalUploadLength;
    ULONG totalWriteLength;
    ULONG numberOfBytesRead;
    ULONG httpStatus = 0;
    BYTE buffer[PAGE_SIZE];

    *Id = NULL;
    *Finished = FALSE;

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
        goto CleanupExit;

    if (fileSize.QuadPart > ScanMaxFileSize)
    {
        status = STATUS_FILE_TOO_LARGE;
        goto CleanupExit;
    }

    baseFileName = PhGetBaseName(FileName);

    {
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"--");
        PhInitFormatI64U(&format[1], PhGenerateRandomNumber64());

        postBoundary = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }

    PhInitializeStringBuilder(&httpRequestHeaders, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostHeader, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostFooter, DOS_MAX_PATH_LENGTH);
    buildersInitialized = TRUE;

    PhAppendStringBuilder2(&httpRequestHeaders, L"accept: application/json\r\n");
    if (PhIsNullOrEmptyString(ApiKey))
    {
        PPH_STRING clientIdHeader = ClientIdHeaderString();
        PhAppendStringBuilder(&httpRequestHeaders, &clientIdHeader->sr);
        PhAppendStringBuilder2(&httpRequestHeaders, L"\r\n");
        PhDereferenceObject(clientIdHeader);
    }
    else
    {
        PhAppendStringBuilder2(&httpRequestHeaders, L"api-key: ");
        PhAppendStringBuilder(&httpRequestHeaders, &ApiKey->sr);
        PhAppendStringBuilder2(&httpRequestHeaders, L"\r\n");
    }
    PhAppendStringBuilder2(&httpRequestHeaders, L"Content-Type: multipart/form-data; boundary=");
    PhAppendStringBuilder(&httpRequestHeaders, &postBoundary->sr);
    PhAppendStringBuilder2(&httpRequestHeaders, L"\r\n");

    PhAppendStringBuilder2(&httpPostHeader, L"--");
    PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
    PhAppendStringBuilder2(&httpPostHeader, L"\r\n");
    PhAppendStringBuilder2(&httpPostHeader, L"Content-Disposition: form-data; name=\"scan_type\"\r\n\r\nall\r\n");

    PhAppendStringBuilder2(&httpPostHeader, L"--");
    PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
    PhAppendStringBuilder2(&httpPostHeader, L"\r\n");
    PhAppendFormatStringBuilder(
        &httpPostHeader,
        L"Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n",
        PhGetStringOrEmpty(baseFileName)
        );
    PhAppendStringBuilder2(&httpPostHeader, L"Content-Type: application/octet-stream\r\n\r\n");

    PhAppendStringBuilder2(&httpPostFooter, L"\r\n--");
    PhAppendStringBuilder(&httpPostFooter, &postBoundary->sr);
    PhAppendStringBuilder2(&httpPostFooter, L"--\r\n");

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(ApiKey))
    {
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"systeminformer.io", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", L"/onlinechecks/hybrid-analysis/api/v2/quick-scan/file", PH_HTTP_FLAG_SECURE)))
            goto CleanupExit;
    }
    else
    {
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"hybrid-analysis.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", L"/api/v2/quick-scan/file", PH_HTTP_FLAG_SECURE)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(
        httpContext,
        httpRequestHeaders.String->Buffer,
        (ULONG)httpRequestHeaders.String->Length / sizeof(WCHAR)
        )))
    {
        goto CleanupExit;
    }

    asciiPostData = PhConvertStringToUtf8(httpPostHeader.String);
    asciiFooterData = PhConvertStringToUtf8(httpPostFooter.String);

    if (!NT_SUCCESS(status = RtlULongAdd((ULONG)asciiPostData->Length, (ULONG)fileSize.QuadPart, &totalUploadLength)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlULongAdd(totalUploadLength, (ULONG)asciiFooterData->Length, &totalUploadLength)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, totalUploadLength)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhHttpWriteData(httpContext, asciiPostData->Buffer, (ULONG)asciiPostData->Length, &totalWriteLength)))
        goto CleanupExit;

    for (;;)
    {
        status = PhReadFile(fileHandle, buffer, PAGE_SIZE, NULL, &numberOfBytesRead);

        if (status == STATUS_END_OF_FILE)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (!NT_SUCCESS(status = PhHttpWriteData(httpContext, buffer, numberOfBytesRead, &totalWriteLength)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpWriteData(httpContext, asciiFooterData->Buffer, (ULONG)asciiFooterData->Length, &totalWriteLength)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus)))
        goto CleanupExit;

    if (httpStatus != 200 && httpStatus != 201)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
        goto CleanupExit;

    *Id = PhGetJsonValueAsString(jsonRootObject, "id");
    *Finished = !!PhGetJsonValueAsUInt64(jsonRootObject, "finished");

    if (PhIsNullOrEmptyString(*Id))
    {
        PhClearReference(Id);
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

CleanupExit:

    if (buildersInitialized)
    {
        PhDeleteStringBuilder(&httpRequestHeaders);
        PhDeleteStringBuilder(&httpPostHeader);
        PhDeleteStringBuilder(&httpPostFooter);
    }

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);
    PhClearReference(&asciiPostData);
    PhClearReference(&asciiFooterData);
    PhClearReference(&postBoundary);
    PhClearReference(&baseFileName);

    if (httpContext)
        PhHttpDestroy(httpContext);

    if (fileHandle)
        NtClose(fileHandle);

    return status;
}

NTSTATUS HybridAnalysisSubmitFinished(
    _In_ PPH_STRING Id,
    _In_opt_ PPH_STRING ApiKey,
    _Out_ PBOOLEAN Finished
    )
{
    NTSTATUS status;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpPathString = NULL;
    PPH_STRING httpHeaderString = NULL;
    PPH_BYTES jsonString = NULL;
    PVOID jsonRootObject = NULL;
    ULONG httpStatus = 0;

    *Finished = FALSE;

    if (PhIsNullOrEmptyString(Id))
        return STATUS_INVALID_PARAMETER_1;

    if (PhIsNullOrEmptyString(ApiKey))
        httpPathString = PhConcatStrings(2, L"/onlinechecks/hybrid-analysis/api/v2/quick-scan/", PhGetString(Id));
    else
        httpPathString = PhConcatStrings(2, L"/api/v2/quick-scan/", PhGetString(Id));

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(ApiKey))
    {
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"systeminformer.io", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;
    }
    else
    {
        if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"hybrid-analysis.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(httpPathString), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(ApiKey))
    {
        httpHeaderString = ClientIdHeaderString();
        if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
            goto CleanupExit;
    }
    else
    {
        httpHeaderString = PhConcatStrings(2, L"api-key: ", PhGetString(ApiKey));
        if (!NT_SUCCESS(status = PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(httpContext, L"accept: application/json", 0)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus)))
        goto CleanupExit;

    if (httpStatus != 200 && httpStatus != 201)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
        goto CleanupExit;

    *Finished = !!PhGetJsonValueAsUInt64(jsonRootObject, "finished");

CleanupExit:

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);
    PhClearReference(&httpHeaderString);
    PhClearReference(&httpPathString);

    if (httpContext)
        PhHttpDestroy(httpContext);

    return status;
}
