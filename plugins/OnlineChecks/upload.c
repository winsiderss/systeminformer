/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2018
 *
 */

#include "onlnchk.h"

PPH_OBJECT_TYPE UploadContextType = NULL;
PH_INITONCE UploadContextTypeInitOnce = PH_INITONCE_INIT;
SERVICE_INFO UploadServiceInfo[] =
{ 
    { MENUITEM_HYBRIDANALYSIS_UPLOAD, L"www.hybrid-analysis.com", L"/api/v2/submit/file", L"file" },
    { MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE, L"www.hybrid-analysis.com", L"/api/v2/submit/file", L"file" },
    { MENUITEM_VIRUSTOTAL_UPLOAD, L"www.virustotal.com", L"???", L"file" },
    { MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE, L"www.virustotal.com", L"???", L"file" },
    { MENUITEM_JOTTI_UPLOAD, L"virusscan.jotti.org", L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
    { MENUITEM_JOTTI_UPLOAD_SERVICE, L"virusscan.jotti.org", L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
};

VOID RaiseUploadError(
    _In_ PUPLOAD_CONTEXT Context,
    _In_ PWSTR Error,
    _In_ ULONG ErrorCode
    )
{
    PPH_STRING message;

    if (!Context->DialogHandle)
        return;

    if (message = PhHttpSocketGetErrorMessage(ErrorCode))
    {
        PhMoveReference(&Context->ErrorString, PhFormatString(
            L"[%lu] %s",
            ErrorCode,
            PhGetString(message)
            ));

        PhDereferenceObject(message);
    }
    else
    {
        PhMoveReference(&Context->ErrorString, PhFormatString(
            L"[%lu] %s",
            ErrorCode,
            Error
            ));
    }

    PostMessage(Context->DialogHandle, UM_ERROR, 0, 0);
}

PSERVICE_INFO GetUploadServiceInfo(
    _In_ ULONG Id
    )
{
    ULONG i;

    for (i = 0; i < ARRAYSIZE(UploadServiceInfo); i++)
    {
        if (UploadServiceInfo[i].Id == Id)
            return &UploadServiceInfo[i];
    }

    return NULL;
}

VOID UploadContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PUPLOAD_CONTEXT context = Object;

    if (context->TaskbarListClass)
    {
        ITaskbarList3_Release(context->TaskbarListClass);
        context->TaskbarListClass = NULL;
    }

    PhClearReference(&context->ErrorString);
    PhClearReference(&context->FileName);
    PhClearReference(&context->BaseFileName);
    PhClearReference(&context->LaunchCommand);
    PhClearReference(&context->Detected);
    PhClearReference(&context->MaxDetected);
    PhClearReference(&context->UploadUrl);
    PhClearReference(&context->ReAnalyseUrl);
    PhClearReference(&context->FirstAnalysisDate);
    PhClearReference(&context->LastAnalysisDate);
    PhClearReference(&context->LastAnalysisUrl);
    PhClearReference(&context->LastAnalysisAgo);
}

VOID TaskDialogFreeContext(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    // Reset Taskbar progress state(s)
    if (Context->TaskbarListClass)
        ITaskbarList3_SetProgressState(Context->TaskbarListClass, PhMainWndHandle, TBPF_NOPROGRESS);

    if (Context->TaskbarListClass)
        ITaskbarList3_SetProgressState(Context->TaskbarListClass, Context->DialogHandle, TBPF_NOPROGRESS);

    PhDereferenceObject(Context);
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG revisionVersion;
    PPH_STRING currentVersion;
    PPH_STRING versionHeader = NULL;

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, NULL, &revisionVersion);

    if (currentVersion = PhFormatString(L"%lu.%lu.%lu", majorVersion, minorVersion, revisionVersion))
    {
        versionHeader = PhConcatStrings2(L"ProcessHacker-Build: ", currentVersion->Buffer);
        PhDereferenceObject(currentVersion);
    }

    return versionHeader;
}

_Success_(return >= 0)
NTSTATUS HashFileAndResetPosition(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER FileSize,
    _In_ PH_HASH_ALGORITHM Algorithm,
    _Out_ PPH_STRING *HashString
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK iosb;
    PH_HASH_CONTEXT hashContext;
    PPH_STRING hashString = NULL;
    ULONG64 bytesRemaining;
    LARGE_INTEGER position;
    KPRIORITY priority;
    IO_PRIORITY_HINT ioPriority;
    BYTE buffer[PAGE_SIZE];
    
    bytesRemaining = FileSize->QuadPart;

    PhGetThreadBasePriority(NtCurrentThread(), &priority);
    PhGetThreadIoPriority(NtCurrentThread(), &ioPriority);
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_LOWEST);
    PhSetThreadIoPriority(NtCurrentThread(), IoPriorityVeryLow);

    PhInitializeHash(&hashContext, Algorithm);

    while (bytesRemaining)
    {
        status = NtReadFile(
            FileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            buffer,
            sizeof(buffer),
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        PhUpdateHash(&hashContext, buffer, (ULONG)iosb.Information);
        bytesRemaining -= (ULONG)iosb.Information;
    }

    if (status == STATUS_END_OF_FILE)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        BYTE hash[32];

        switch (Algorithm)
        {
        case Md5HashAlgorithm:
            PhFinalHash(&hashContext, hash, 16, NULL);
            *HashString = PhBufferToHexString(hash, 16);
            break;
        case Sha1HashAlgorithm:
            PhFinalHash(&hashContext, hash, 20, NULL);
            *HashString = PhBufferToHexString(hash, 20);
            break;
        case Sha256HashAlgorithm:
            PhFinalHash(&hashContext, hash, 32, NULL);
            *HashString = PhBufferToHexString(hash, 32);
            break;
        }

        position.QuadPart = 0;
        status = PhSetFilePosition(FileHandle, &position);
    }

    PhSetThreadBasePriority(NtCurrentThread(), priority);
    PhSetThreadIoPriority(NtCurrentThread(), ioPriority);

    return status;
}

PPH_BYTES PerformSubRequest(
    _In_ PUPLOAD_CONTEXT Context,
    _In_ PWSTR HostName,
    _In_ PWSTR ObjectName
    )
{
    PPH_BYTES result = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING phVersion;
    PPH_STRING userAgent;

    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"ProcessHacker_", phVersion->Buffer);

    if (!PhHttpSocketCreate(&httpContext, userAgent->Buffer))
    {
        RaiseUploadError(Context, L"Unable to create the http socket.", GetLastError());
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(httpContext, HostName, PH_HTTP_DEFAULT_HTTPS_PORT))
    {
        RaiseUploadError(Context, L"Unable to connect to the service.", GetLastError());
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        NULL,
        ObjectName,
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        RaiseUploadError(Context, L"Unable to create the request.", GetLastError());
        goto CleanupExit;
    }

    if (
        Context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD ||
        Context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE
        )
    {
        PPH_BYTES serviceHash;
        PPH_STRING httpHeader;

        serviceHash = VirusTotalGetCachedDbHash(&ServiceObjectDbHash);
        httpHeader = PhFormatString(
            L"\x0061\x0070\x0069\x002D\x006B\x0065\x0079:%hs",
            serviceHash->Buffer
            );

        PhHttpSocketAddRequestHeaders(
            httpContext,
            httpHeader->Buffer,
            (ULONG)httpHeader->Length / sizeof(WCHAR)
            );

        PhClearReference(&httpHeader);
        PhClearReference(&serviceHash);
    }

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
    {
        RaiseUploadError(Context, L"Unable to send the request.", GetLastError());
        goto CleanupExit;
    }

    if (!PhHttpSocketEndRequest(httpContext))
    {
        RaiseUploadError(Context, L"Unable to receive the request.", GetLastError());
        goto CleanupExit;
    }

    if (!(result = PhHttpSocketDownloadString(httpContext, FALSE)))
    {
        RaiseUploadError(Context, L"Unable to download the response.", GetLastError());
        goto CleanupExit;
    }

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    PhClearReference(&phVersion);
    PhClearReference(&userAgent);

    return result;
}

NTSTATUS UploadFileThreadStart(
    _In_ PVOID Parameter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG httpStatus = 0;
    ULONG httpPostSeed = 0;
    ULONG totalUploadLength = 0;
    ULONG totalUploadedLength = 0;
    ULONG totalPostHeaderWritten = 0;
    ULONG totalPostFooterWritten = 0;
    ULONG totalWriteLength = 0;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK isb;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING phVersion = NULL;
    PPH_STRING userAgent = NULL;
    PPH_STRING httpHostName = NULL;
    PPH_STRING httpHostPath = NULL;
    USHORT httpHostPort = 0;
    PSERVICE_INFO serviceInfo = NULL;
    PPH_STRING postBoundary = NULL;
    PPH_BYTES asciiPostData = NULL;
    PPH_BYTES asciiFooterData = NULL;
    PH_STRING_BUILDER httpRequestHeaders = { 0 };
    PH_STRING_BUILDER httpPostHeader = { 0 };
    PH_STRING_BUILDER httpPostFooter = { 0 };
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    serviceInfo = GetUploadServiceInfo(context->Service);

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(context->FileName),
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        RaiseUploadError(context, L"Unable to open the file", PhNtStatusToDosError(status));
        goto CleanupExit;
    }

    // Create a user agent string.
    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"ProcessHacker_", phVersion->Buffer);

    if (!PhHttpSocketCreate(&httpContext, userAgent->Buffer))
    {
        PhClearReference(&phVersion);
        PhClearReference(&userAgent);
        goto CleanupExit;
    }

    PhClearReference(&phVersion);
    PhClearReference(&userAgent);

    if (!PhHttpSocketParseUrl(
        context->UploadUrl,
        &httpHostName,
        &httpHostPath,
        &httpHostPort
        ))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        PhGetString(httpHostName),
        httpHostPort
        ))
    {
        RaiseUploadError(context, L"Unable to connect to the service", GetLastError());
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        PhGetString(httpHostPath),
        PH_HTTP_FLAG_REFRESH | (httpHostPort == PH_HTTP_DEFAULT_HTTPS_PORT ? PH_HTTP_FLAG_SECURE : 0)
        ))
    {
        RaiseUploadError(context, L"Unable to create the request", GetLastError());
        goto CleanupExit;
    }

    PhInitializeStringBuilder(&httpRequestHeaders, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostHeader, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostFooter, DOS_MAX_PATH_LENGTH);

    // HTTP request boundary string.
    postBoundary = PhFormatString(
        L"--%I64u",
        (ULONG64)RtlRandomEx(&httpPostSeed) | ((ULONG64)RtlRandomEx(&httpPostSeed) << 31)
        );

    if (
        context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD || 
        context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE
        )
    {
        USHORT machineType;
        USHORT environmentId;
        PH_MAPPED_IMAGE mappedImage;
        PPH_BYTES serviceHash;

        if (!NT_SUCCESS(status = PhLoadMappedImageEx(NULL, fileHandle, &mappedImage)))
        {
            RaiseUploadError(context, L"Unable to load the image.", PhNtStatusToDosError(status));
            goto CleanupExit;
        }

        switch (mappedImage.Signature)
        {
        case IMAGE_DOS_SIGNATURE:
            machineType = mappedImage.NtHeaders->FileHeader.Machine;
            break;
        case IMAGE_ELF_SIGNATURE:
            machineType = USHRT_MAX; // Windows only supports 64bit ELF. (mappedImage.Header->e_machine)
            break;
        }

        PhUnloadMappedImage(&mappedImage);

        switch (machineType)
        {
        case IMAGE_FILE_MACHINE_I386:
            environmentId = 110;
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            environmentId = 120;
            break;
        case USHRT_MAX: // 64bit Linux 
            environmentId = 300;
            break;
        default:
            {
                RaiseUploadError(context, L"File architecture not supported.", STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT);
                goto CleanupExit;
            }
        }

        // HTTP request headers
        PhAppendFormatStringBuilder(
            &httpRequestHeaders,
            L"accept: application/json\r\n"
            );

        serviceHash = VirusTotalGetCachedDbHash(&ServiceObjectDbHash);
        PhAppendFormatStringBuilder(
            &httpRequestHeaders,
            L"\x0061\x0070\x0069\x002D\x006B\x0065\x0079:%hs\r\n",
            serviceHash->Buffer
            );
        PhClearReference(&serviceHash);

        PhAppendFormatStringBuilder(
            &httpRequestHeaders,
            L"Content-Type: multipart/form-data; boundary=%s\r\n",
            postBoundary->Buffer
            );

        // POST boundary header.
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"--%s\r\nContent-Disposition: form-data; name=\"environment_id\"\r\n\r\n%hu\r\n",
            postBoundary->Buffer,
            environmentId
            );
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n\r\n",
            postBoundary->Buffer,
            PhGetStringOrEmpty(context->BaseFileName)
            );

        // POST boundary footer.
        PhAppendFormatStringBuilder(
            &httpPostFooter,
            L"\r\n--%s--\r\n",
            postBoundary->Buffer
            );
    }
    else if (context->Service == MENUITEM_JOTTI_UPLOAD)
    {
        PhAppendFormatStringBuilder(
            &httpRequestHeaders,
            L"Content-Type: multipart/form-data; boundary=%s\r\n",
            postBoundary->Buffer
            );

        // POST boundary header.
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"\r\n--%s\r\n",
            postBoundary->Buffer
            );
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"MAX_FILE_SIZE\"\r\n\r\n268435456\r\n"
            );
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"--%s\r\n",
            postBoundary->Buffer
            );
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",
            serviceInfo->FileNameFieldName,
            PhGetStringOrEmpty(context->BaseFileName)
            );
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Type: application/x-msdownload\r\n\r\n"
            );

        // POST boundary footer.
        PhAppendFormatStringBuilder(
            &httpPostFooter,
            L"\r\n--%s--\r\n",
            postBoundary->Buffer
            );
    }
    else
    {
        PhAppendFormatStringBuilder(
            &httpRequestHeaders,
            L"Content-Type: multipart/form-data; boundary=%s\r\n",
            postBoundary->Buffer
            );

        // POST boundary header
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"--%s\r\n",
            postBoundary->Buffer
            );
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",
            serviceInfo->FileNameFieldName,
            PhGetStringOrEmpty(context->BaseFileName)
            );
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Type: application/octet-stream\r\n\r\n"
            );

        // POST boundary footer
        PhAppendFormatStringBuilder(
            &httpPostFooter,
            L"\r\n--%s--\r\n\r\n",
            postBoundary->Buffer
            );
    }

    // add headers
    if (!PhHttpSocketAddRequestHeaders(
        httpContext,
        httpRequestHeaders.String->Buffer,
        (ULONG)httpRequestHeaders.String->Length / sizeof(WCHAR)
        ))
    {
        RaiseUploadError(context, L"Unable to add request headers", GetLastError());
        goto CleanupExit;
    }

    if (context->Service == MENUITEM_JOTTI_UPLOAD)
    {
        PPH_STRING ajaxHeader = PhCreateString(L"X-Requested-With: XMLHttpRequest");

        PhHttpSocketAddRequestHeaders(
            httpContext,
            ajaxHeader->Buffer,
            (ULONG)ajaxHeader->Length / sizeof(WCHAR)
            );

        PhDereferenceObject(ajaxHeader);
    }

    // Calculate the total request length.
    totalUploadLength = (ULONG)httpPostHeader.String->Length / sizeof(WCHAR) + context->TotalFileLength + (ULONG)httpPostFooter.String->Length / sizeof(WCHAR);

    // Send the request.
    if (!PhHttpSocketSendRequest(httpContext, NULL, totalUploadLength))
    {
        RaiseUploadError(context, L"Unable to send the request", GetLastError());
        goto CleanupExit;
    }

    // Convert to ASCII
    asciiPostData = PhConvertUtf16ToAscii(httpPostHeader.String->Buffer, '-');
    asciiFooterData = PhConvertUtf16ToAscii(httpPostFooter.String->Buffer, '-');

    // Start the clock.
    PhQuerySystemTime(&timeStart);

    // Write the header
    if (!PhHttpSocketWriteData(
        httpContext,
        asciiPostData->Buffer,
        (ULONG)asciiPostData->Length,
        &totalPostHeaderWritten
        ))
    {
        RaiseUploadError(context, L"Unable to write the post header", GetLastError());
        goto CleanupExit;
    }

    PPH_STRING msg = PhFormatString(L"Uploading %s...", PhGetStringOrEmpty(context->BaseFileName));
    SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhGetString(msg));
    PhDereferenceObject(msg);

    if (context->TaskbarListClass)
    {
        ITaskbarList3_SetProgressState(
            context->TaskbarListClass,
            PhMainWndHandle,
            TBPF_NORMAL
            );
    }

    while (TRUE)
    {
        BYTE buffer[PAGE_SIZE];

        if (context->Cancel)
        {
            RaiseUploadError(context, L"Unable to complete the request.", STATUS_CANCELLED);
            goto CleanupExit;
        }

        if (!NT_SUCCESS(status = NtReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            PAGE_SIZE,
            NULL,
            NULL
            )))
        {
            break;
        }

        if (!PhHttpSocketWriteData(httpContext, buffer, (ULONG)isb.Information, &totalWriteLength))
        {
            RaiseUploadError(context, L"Unable to upload the file data", GetLastError());
            goto CleanupExit;
        }

        PhQuerySystemTime(&timeNow);

        totalUploadedLength += totalWriteLength;
        timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
        timeBitsPerSecond = totalUploadedLength / __max(timeTicks, 1);

        {
            FLOAT percent = ((FLOAT)totalUploadedLength / context->TotalFileLength * 100);
            PH_FORMAT format[9];
            WCHAR string[MAX_PATH];

            // L"Uploaded: %s of %s (%.0f%%)\r\nSpeed: %s/s"
            PhInitFormatS(&format[0], L"Uploaded: ");
            PhInitFormatSize(&format[1], totalUploadedLength);
            PhInitFormatS(&format[2], L" of ");
            PhInitFormatSize(&format[3], context->TotalFileLength);
            PhInitFormatS(&format[4], L" (");
            PhInitFormatF(&format[5], percent, 1);
            PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
            PhInitFormatSize(&format[7], timeBitsPerSecond);
            PhInitFormatS(&format[8], L"/s");

            if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
            {
                SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
            }

            SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);

            if (context->TaskbarListClass)
            {
                ITaskbarList3_SetProgressValue(
                    context->TaskbarListClass, 
                    context->DialogHandle, 
                    totalUploadedLength, 
                    context->TotalFileLength
                    );
            }
        }
    }

    // Write the footer bytes
    if (!PhHttpSocketWriteData(
        httpContext,
        asciiFooterData->Buffer,
        (ULONG)asciiFooterData->Length,
        &totalPostFooterWritten
        ))
    {
        RaiseUploadError(context, L"Unable to write the post footer", GetLastError());
        goto CleanupExit;
    }

    if (!PhHttpSocketEndRequest(httpContext))
    {
        RaiseUploadError(context, L"Unable to receive the response", GetLastError());
        goto CleanupExit;
    }

    if (!PhHttpSocketQueryHeaderUlong(
        httpContext,
        PH_HTTP_QUERY_STATUS_CODE,
        &httpStatus
        ))
    {
        RaiseUploadError(context, L"Unable to query http headers", GetLastError());
        goto CleanupExit;
    }

    if (
        httpStatus == PH_HTTP_STATUS_OK || 
        httpStatus == PH_HTTP_STATUS_CREATED || 
        httpStatus == PH_HTTP_STATUS_REDIRECT_METHOD || 
        httpStatus == PH_HTTP_STATUS_REDIRECT
        )
    {
        switch (context->Service)
        {
        case MENUITEM_HYBRIDANALYSIS_UPLOAD:
        case MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE:
            {
                PPH_BYTES jsonString;
                PVOID jsonRootObject;
                INT64 errorCode;

                if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
                {
                    RaiseUploadError(context, L"Unable to download the response.", GetLastError());
                    goto CleanupExit;
                }

                if (jsonRootObject = PhCreateJsonParser(jsonString->Buffer))
                {
                    errorCode = PhGetJsonValueAsUInt64(jsonRootObject, "code");

                    if (errorCode == 0)
                    {
                        PPH_STRING jsonHashString;
                        //PPH_STRING jsonJobIdString;

                        jsonHashString = PhGetJsonValueAsString(jsonRootObject, "sha256");
                        //jsonJobIdString = PhGetJsonValueAsString(jsonRootObject, "job_id");

                        if (jsonHashString) // && jsonJobIdString)
                        {
                            PhMoveReference(&context->LaunchCommand, PhFormatString(
                                L"https://www.hybrid-analysis.com/sample/%s", // /%s
                                jsonHashString->Buffer//,
                                //jsonJobIdString->Buffer
                                ));
                        }

                        if (jsonHashString)
                            PhDereferenceObject(jsonHashString);
                        //if (jsonJobIdString)
                        //    PhDereferenceObject(jsonJobIdString);
                    }
                    else
                    {
                        RaiseUploadError(context, L"Hybrid Analysis API error.", (ULONG)errorCode);
                        PhDereferenceObject(jsonString);
                        goto CleanupExit;
                    }

                    PhFreeJsonObject(jsonRootObject);
                }
                else
                {
                    RaiseUploadError(context, L"Unable to complete the request.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
                    goto CleanupExit;
                }

                PhDereferenceObject(jsonString);
            }
            break;
        case MENUITEM_VIRUSTOTAL_UPLOAD:
        case MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE:
            {
                PPH_BYTES jsonString;
                PVOID jsonRootObject;
                PVOID jsonDataObject;

                if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
                {
                    RaiseUploadError(context, L"Unable to complete the request.", GetLastError());
                    goto CleanupExit;
                }

                if (!PhIsNullOrEmptyString(context->FileHash))
                {
                    PVIRUSTOTAL_FILE_REPORT fileReport;

                    if (fileReport = VirusTotalRequestFileReport(context->FileHash))
                    {
                        VirusTotalFreeFileReport(fileReport);
                    }
                }

                if (jsonRootObject = PhCreateJsonParser(jsonString->Buffer))
                {
                    PPH_STRING permalink = PhGetJsonValueAsString(jsonRootObject, "permalink");

                    if (PhIsNullOrEmptyString(permalink))
                    {
                        if (jsonDataObject = PhGetJsonObject(jsonRootObject, "data"))
                        {
                            PPH_STRING analysisId = PhGetJsonValueAsString(jsonDataObject, "id");

                            PhMoveReference(&context->LaunchCommand, PhFormatString(
                                L"https://www.virustotal.com/#/file-analysis/%s",
                                analysisId->Buffer
                                ));

                            PhDereferenceObject(analysisId);
                        }
                        else
                        {
                            if (PhGetJsonValueAsUInt64(jsonRootObject, "response_code") == 1)
                            {
                                PhMoveReference(&context->LaunchCommand, PhGetJsonValueAsString(jsonRootObject, "permalink"));
                            }
                        }
                    }
                    else
                    {
                        PhMoveReference(&context->LaunchCommand, PhGetJsonValueAsString(jsonRootObject, "permalink"));
                    }

                    PhFreeJsonObject(jsonRootObject);
                }
                
                if (PhIsNullOrEmptyString(context->LaunchCommand))
                {
                    RaiseUploadError(context, L"Unable to complete the request.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
                    PhDereferenceObject(jsonString);
                    goto CleanupExit;
                }

                PhDereferenceObject(jsonString);
            }
            break;
        case MENUITEM_JOTTI_UPLOAD:
        case MENUITEM_JOTTI_UPLOAD_SERVICE:
            {
                PPH_BYTES jsonString;
                PVOID jsonRootObject;

                if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
                {
                    RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                    goto CleanupExit;
                }

                if (jsonRootObject = PhCreateJsonParser(jsonString->Buffer))
                {
                    PPH_STRING redirectUrl;

                    if (redirectUrl = PhGetJsonValueAsString(jsonRootObject, "redirecturl"))
                    {
                        PhMoveReference(&context->LaunchCommand, PhFormatString(L"http://virusscan.jotti.org%s", redirectUrl->Buffer));
                        PhDereferenceObject(redirectUrl);
                    }

                    PhFreeJsonObject(jsonRootObject);
                }

                PhDereferenceObject(jsonString);
            }
            break;
        }
    }
    else
    {
        RaiseUploadError(context, L"Unable to complete the request.", httpStatus);
        goto CleanupExit;
    }

    if (context->Cancel)
    {
        RaiseUploadError(context, L"Unable to complete the request.", STATUS_CANCELLED);
        goto CleanupExit;
    }

    if (!PhIsNullOrEmptyString(context->LaunchCommand))
    {
        PostMessage(context->DialogHandle, UM_LAUNCH, 0, 0);
    }
    else
    {
        RaiseUploadError(context, L"Unable to complete the Launch request (please try again after a few minutes)", ERROR_INVALID_DATA);
    }

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    // Reset Taskbar progress state(s)
    if (context->TaskbarListClass)
    {
        ITaskbarList3_SetProgressState(context->TaskbarListClass, PhMainWndHandle, TBPF_NOPROGRESS);
        ITaskbarList3_SetProgressState(context->TaskbarListClass, context->DialogHandle, TBPF_NOPROGRESS);
    }

    if (postBoundary)
        PhDereferenceObject(postBoundary);

    if (asciiFooterData)
        PhDereferenceObject(asciiFooterData);

    if (asciiPostData)
        PhDereferenceObject(asciiPostData);

    if (httpPostFooter.String)
        PhDeleteStringBuilder(&httpPostFooter);

    if (httpPostHeader.String)
        PhDeleteStringBuilder(&httpPostHeader);

    if (httpRequestHeaders.String)
        PhDeleteStringBuilder(&httpRequestHeaders);

    if (fileHandle)
        NtClose(fileHandle);

    PhDereferenceObject(context);

    return status;
}

NTSTATUS UploadCheckThreadStart(
    _In_ PVOID Parameter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN fileExists = FALSE;
    LARGE_INTEGER fileSize64;
    PPH_BYTES subRequestBuffer = NULL;
    PSERVICE_INFO serviceInfo = NULL;
    PPH_STRING subObjectName = NULL;
    HANDLE fileHandle = NULL;
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    //context->Extension = VirusTotalGetCachedResult(context->FileName);
    serviceInfo = GetUploadServiceInfo(context->Service);

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(context->FileName),
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        RaiseUploadError(context, L"Unable to open the file", PhNtStatusToDosError(status));
        goto CleanupExit;
    }

    if (NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize64)))
    {
        if (context->Service == MENUITEM_VIRUSTOTAL_UPLOAD || 
            context->Service == MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE)
        {
            if (fileSize64.QuadPart < 32 * 1024 * 1024)
            {
                context->VtApiUpload = TRUE;
            }
            
            if (fileSize64.QuadPart > 128 * 1024 * 1024) // 128 MB
            {
                RaiseUploadError(context, L"The file is too large (over 128 MB)", ERROR_FILE_TOO_LARGE);
                goto CleanupExit;
            }
        }
        else if (
            context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD ||
            context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE
            )
        {
            if (fileSize64.QuadPart > 128 * 1024 * 1024) // 128 MB
            {
                RaiseUploadError(context, L"The file is too large (over 128 MB)", ERROR_FILE_TOO_LARGE);
                goto CleanupExit;
            }
        }
        else
        {
            if (fileSize64.QuadPart > 20 * 1024 * 1024) // 20 MB
            {
                RaiseUploadError(context, L"The file is too large (over 20 MB)", ERROR_FILE_TOO_LARGE);
                goto CleanupExit;
            }
        }

        context->FileSize = PhFormatSize(fileSize64.QuadPart, ULONG_MAX);
        context->TotalFileLength = fileSize64.LowPart;
    }

    switch (context->Service)
    {
    case MENUITEM_HYBRIDANALYSIS_UPLOAD:
    case MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE:
        {
            PPH_STRING tempHashString = NULL;
            PSTR uploadUrl = NULL;
            PSTR quote = NULL;
            PVOID rootJsonObject;

            // Create the default upload URL.
            context->UploadUrl = PhFormatString(
                L"https://%s%s", 
                serviceInfo->HostName, 
                serviceInfo->UploadObjectName
                );

            if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, Sha256HashAlgorithm, &tempHashString)))
            {
                RaiseUploadError(context, L"Unable to hash the file", PhNtStatusToDosError(status));
                goto CleanupExit;
            }

            context->FileHash = tempHashString;
            subObjectName = PhConcatStrings2(L"/api/v2/overview/", PhGetString(context->FileHash));

            if (!(subRequestBuffer = PerformSubRequest(
                context,
                serviceInfo->HostName,
                subObjectName->Buffer
                )))
            {
                goto CleanupExit;
            }

            if (rootJsonObject = PhCreateJsonParser(subRequestBuffer->Buffer))
            {
                PPH_STRING errorMessage = PhGetJsonValueAsString(rootJsonObject, "message");

                if (context->FileExists = PhIsNullOrEmptyString(errorMessage))
                {
                    PhMoveReference(&context->LaunchCommand, PhFormatString(
                        L"https://www.hybrid-analysis.com/sample/%s",
                        PhGetString(context->FileHash)
                        ));

                    PostMessage(context->DialogHandle, UM_LAUNCH, 0, 0);
                }
                else
                {
                    PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
                }

                PhClearReference(&errorMessage);
                PhFreeJsonObject(rootJsonObject);
            }
            else
            {
                RaiseUploadError(context, L"Unable to parse the response.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
            }
        }
        break;
    case MENUITEM_VIRUSTOTAL_UPLOAD:
    case MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE:
        {
            PPH_STRING tempHashString = NULL;
            PSTR uploadUrl = NULL;
            PSTR quote = NULL;
            PVOID rootJsonObject;

            if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, Sha256HashAlgorithm, &tempHashString)))
            {
                RaiseUploadError(context, L"Unable to hash the file", PhNtStatusToDosError(status));
                goto CleanupExit;
            }

            context->FileHash = tempHashString;
            subObjectName = PhConcatStrings2(L"/file/upload/?sha256=", PhGetString(context->FileHash));

            if (!(subRequestBuffer = PerformSubRequest(
                context,
                serviceInfo->HostName,
                subObjectName->Buffer
                )))
            {
                goto CleanupExit;
            }

            if (rootJsonObject = PhCreateJsonParser(subRequestBuffer->Buffer))
            {  
                if (context->FileExists = PhGetJsonObjectBool(rootJsonObject, "file_exists"))
                {
                    INT64 detected = 0;
                    INT64 detectedMax = 0;
                    PVOID detectionRatio;

                    if (detectionRatio = PhGetJsonObject(rootJsonObject, "detection_ratio"))
                    {
                        detected = PhGetJsonArrayLong64(detectionRatio, 0);
                        detectedMax = PhGetJsonArrayLong64(detectionRatio, 1);
                    }

                    context->Detected = PhFormatUInt64(detected, FALSE);
                    context->MaxDetected = PhFormatUInt64(detectedMax, FALSE);
                    context->UploadUrl = PhGetJsonValueAsString(rootJsonObject, "upload_url");
                    context->ReAnalyseUrl = PhGetJsonValueAsString(rootJsonObject, "reanalyse_url");
                    context->LastAnalysisUrl = PhGetJsonValueAsString(rootJsonObject, "last_analysis_url");
                    context->FirstAnalysisDate = PhGetJsonValueAsString(rootJsonObject, "first_analysis_date");
                    context->LastAnalysisDate = PhGetJsonValueAsString(rootJsonObject, "last_analysis_date");
                    context->LastAnalysisAgo = PhGetJsonValueAsString(rootJsonObject, "last_analysis_ago");

                    PhMoveReference(&context->FirstAnalysisDate, VirusTotalStringToTime(context->FirstAnalysisDate));
                    PhMoveReference(&context->LastAnalysisDate, VirusTotalStringToTime(context->LastAnalysisDate));
                    
                    if (!PhIsNullOrEmptyString(context->ReAnalyseUrl))
                    {
                        PhMoveReference(&context->ReAnalyseUrl, PhFormatString(
                            L"%s%s", 
                            L"https://www.virustotal.com",
                            PhGetString(context->ReAnalyseUrl)
                            ));
                    }

                    if (context->VtApiUpload)
                    {
                        PPH_BYTES resource = VirusTotalGetCachedDbHash(&ProcessObjectDbHash);

                        PhMoveReference(&context->UploadUrl, PhFormatString(
                            L"%s%s?\x0061\x0070\x0069\x006B\x0065\x0079=%S&resource=%s",
                            L"https://www.virustotal.com",
                            L"/vtapi/v2/file/scan",
                            resource->Buffer,
                            PhGetString(context->FileHash)
                            ));

                        PhClearReference(&resource);
                    }

                    PhMoveReference(&context->LaunchCommand, PhFormatString(
                        L"https://www.virustotal.com/file/%s/analysis/",
                        PhGetString(context->FileHash)
                        ));

                    if (!PhIsNullOrEmptyString(context->UploadUrl))
                    {
                        PostMessage(context->DialogHandle, UM_EXISTS, 0, 0);
                    }
                    else
                    {
                        RaiseUploadError(context, L"Unable to parse the UploadUrl.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
                    }
                }
                else
                {
                    PPH_STRING vt3UploadUrl;
                    PPH_BYTES vt3UploadRequestBuffer;
                    PVOID vt3RootJsonObject;

                    if (context->VtApiUpload)
                    {
                        PPH_BYTES resource = VirusTotalGetCachedDbHash(&ProcessObjectDbHash);

                        PhMoveReference(&context->UploadUrl, PhFormatString(
                            L"%s%s?\x0061\x0070\x0069\x006B\x0065\x0079=%S&resource=%s",
                            L"https://www.virustotal.com",
                            L"/vtapi/v2/file/scan",
                            resource->Buffer,
                            PhGetString(context->FileHash)
                            ));

                        PhClearReference(&resource);
                    }
                    else
                    {
                        vt3UploadUrl = PhCreateString(L"/ui/files/upload_url");

                        if (!(vt3UploadRequestBuffer = PerformSubRequest(
                            context,
                            serviceInfo->HostName,
                            vt3UploadUrl->Buffer
                            )))
                        {
                            PhDereferenceObject(vt3UploadUrl);
                            goto CleanupExit;
                        }

                        if (vt3RootJsonObject = PhCreateJsonParser(vt3UploadRequestBuffer->Buffer))
                        {
                            context->UploadUrl = PhGetJsonValueAsString(vt3RootJsonObject, "data");
                            PhFreeJsonObject(vt3RootJsonObject);
                        }

                        PhClearReference(&vt3UploadRequestBuffer);
                    }

                    // No file found... Start the upload.
                    if (!PhIsNullOrEmptyString(context->UploadUrl))
                    {
                        PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
                    }
                    else
                    {
                        RaiseUploadError(context, L"Received invalid VT3 response.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
                    }
                }
 
                PhFreeJsonObject(rootJsonObject);
            }
            else
            {
                RaiseUploadError(context, L"Unable to parse the response.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
            }
        }
        break;
    case MENUITEM_JOTTI_UPLOAD:
    case MENUITEM_JOTTI_UPLOAD_SERVICE:
        {
            // Create the default upload URL.
            context->UploadUrl = PhFormatString(L"https://%s%s", serviceInfo->HostName, serviceInfo->UploadObjectName);

            // Start the upload.
            PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
        }
        break;
    }

CleanupExit:

    PhClearReference(&subObjectName);
    PhClearReference(&subRequestBuffer);

    if (fileHandle)
        NtClose(fileHandle);

    PhDereferenceObject(context);

    return status;
}

NTSTATUS UploadRecheckThreadStart(
    _In_ PVOID Parameter
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    PVIRUSTOTAL_API_RESPONSE fileRescan;
    
    if (fileRescan = VirusTotalRequestFileReScan(context->FileHash))
    {
        if (fileRescan->ResponseCode == 1)
        {
            PhSwapReference(&context->ReAnalyseUrl, fileRescan->PermaLink);

            PhShellExecute(NULL, PhGetString(context->ReAnalyseUrl), NULL);

            SendMessage(context->DialogHandle, TDM_CLICK_BUTTON, IDOK, 0);
        }
        else
        {
            RaiseUploadError(context, L"VirusTotal ReScan API error.", (ULONG)fileRescan->ResponseCode);
        }

        VirusTotalFreeFileReScan(fileRescan);
    }
    else
    {
        RaiseUploadError(context, L"VirusTotal ReScan API error.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
    }

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

NTSTATUS ViewReportThreadStart(
    _In_ PVOID Parameter
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    PVIRUSTOTAL_FILE_REPORT fileReport;
    
    if (fileReport = VirusTotalRequestFileReport(context->FileHash))
    {
        if (fileReport->ResponseCode == 1)
        {
            PhSwapReference(&context->LaunchCommand, fileReport->PermaLink);

            PhShellExecute(NULL, PhGetString(context->LaunchCommand), NULL);

            SendMessage(context->DialogHandle, TDM_CLICK_BUTTON, IDOK, 0);
        }
        else
        {
            RaiseUploadError(context, L"VirusTotal ViewReport API error.", (ULONG)fileReport->ResponseCode);
        }

        VirusTotalFreeFileReport(fileReport);
    }
    else
    {
        RaiseUploadError(context, L"VirusTotal ViewReport API error.", PhNtStatusToDosError(STATUS_FAIL_CHECK));
    }

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PUPLOAD_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, 0xF)))
        return 0;

    oldWndProc = context->DialogWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwndDlg, 0xF);

            TaskDialogFreeContext(context);
        }
        break;
    case UM_UPLOAD:
        {
            ShowVirusTotalProgressDialog(context);
        }
        break;
    case UM_EXISTS:
        {
            switch (PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_ACTION))
            {
            //default:
            case 1:
                {
                    ShowVirusTotalProgressDialog(context);
                }
                break;
            case 2:
                {
//#ifdef PH_BUILD_API
                    ShowVirusTotalReScanProgressDialog(context);
//#else
//                    if (!PhIsNullOrEmptyString(context->ReAnalyseUrl))
//                    {
//                        PhShellExecute(hwndDlg, PhGetString(context->ReAnalyseUrl), NULL);
//                    }
//#endif
                }
                break;
            case 3:
                {
//#ifdef PH_BUILD_API
                    ShowVirusTotalViewReportProgressDialog(context);
//#else
//                    if (!PhIsNullOrEmptyString(context->LaunchCommand))
//                    {
//                        PhShellExecute(hwndDlg, PhGetString(context->LaunchCommand), NULL);
//                    }
//#endif
                }
                break;
            default:
                {
                    ShowFileFoundDialog(context);
                }
            }
        }
        break;
    case UM_LAUNCH:
        {
            if (!PhIsNullOrEmptyString(context->LaunchCommand))
            {
                PhShellExecute(hwndDlg, context->LaunchCommand->Buffer, NULL);
            }

            SendMessage(hwndDlg, TDM_CLICK_BUTTON, IDOK, 0);
        }
        break;
    case UM_ERROR:
        {
            VirusTotalShowErrorDialog(context);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

HRESULT CALLBACK TaskDialogBootstrapCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            context->DialogHandle = hwndDlg;

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(PhMainWndHandle) && !IsMinimized(PhMainWndHandle)) ? PhMainWndHandle : NULL);

            // Create the Taskdialog icons
            PhSetApplicationWindowIcon(hwndDlg);

            if (SUCCEEDED(PhGetClassObject(L"explorerframe.dll", &CLSID_TaskbarList, &IID_ITaskbarList3, &context->TaskbarListClass)))
            {
                if (FAILED(ITaskbarList3_HrInit(context->TaskbarListClass)))
                {
                    ITaskbarList3_Release(context->TaskbarListClass);
                    context->TaskbarListClass = NULL;
                }
            }

            context->DialogWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, 0xF, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)TaskDialogSubclassProc);

            ShowVirusTotalUploadDialog(context);
        }
        break;
    }

    return S_OK;
}

NTSTATUS ShowUploadDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    BOOL checked = FALSE;

    PhInitializeAutoPool(&autoPool);

    config.hInstance = PluginInstance->DllBase;
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;

    // Start TaskDialog bootstrap
    TaskDialogIndirect(&config, NULL, NULL, &checked);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID UploadToOnlineService(
    _In_ PPH_STRING FileName,
    _In_ ULONG Service
    )
{
    PUPLOAD_CONTEXT context;

    if (PhBeginInitOnce(&UploadContextTypeInitOnce))
    {
        UploadContextType = PhCreateObjectType(L"OnlineChecksObjectType", 0, UploadContextDeleteProcedure);
        PhEndInitOnce(&UploadContextTypeInitOnce);
    }

    PhReferenceObject(FileName);

    context = PhCreateObjectZero(sizeof(UPLOAD_CONTEXT), UploadContextType);
    context->Service = Service;
    context->FileName = FileName;
    context->BaseFileName = PhGetBaseName(context->FileName);

    PhCreateThread2(ShowUploadDialogThread, (PVOID)context);
}

VOID UploadServiceToOnlineService(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG Service
    )
{
    NTSTATUS status;
    PPH_STRING serviceFileName;
    PPH_STRING serviceBinaryPath = NULL;

    if (PhBeginInitOnce(&UploadContextTypeInitOnce))
    {
        UploadContextType = PhCreateObjectType(L"OnlineChecksObjectType", 0, UploadContextDeleteProcedure);
        PhEndInitOnce(&UploadContextTypeInitOnce);
    }

    if (NT_SUCCESS(status = QueryServiceFileName(
        &ServiceItem->Name->sr,
        &serviceFileName,
        &serviceBinaryPath
        )))
    {
        PUPLOAD_CONTEXT context;

        context = PhCreateObjectZero(sizeof(UPLOAD_CONTEXT), UploadContextType);
        context->Service = Service;
        context->FileName = serviceFileName;
        context->BaseFileName = PhGetBaseName(context->FileName);

        PhCreateThread2(ShowUploadDialogThread, (PVOID)context);
    }
    else
    {
        PhShowStatus(PhMainWndHandle, L"Unable to query the service.", status, 0);
    }

    if (serviceBinaryPath)
        PhDereferenceObject(serviceBinaryPath);
}
