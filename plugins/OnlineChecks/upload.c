/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2024
 *
 */

#include "onlnchk.h"
#include <kphuser.h>

PPH_OBJECT_TYPE UploadContextType = NULL;
PH_INITONCE UploadContextTypeInitOnce = PH_INITONCE_INIT;
CONST SERVICE_INFO UploadServiceInfo[] =
{
    { MENUITEM_HYBRIDANALYSIS_UPLOAD, L"hybrid-analysis.com", L"/api/v2/submit/file", L"file" },
    { MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE, L"hybrid-analysis.com", L"/api/v2/submit/file", L"file" },
    { MENUITEM_VIRUSTOTAL_UPLOAD, L"www.virustotal.com", L"???", L"file" },
    { MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE, L"www.virustotal.com", L"???", L"file" },
    { MENUITEM_JOTTI_UPLOAD, L"virusscan.jotti.org", L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
    { MENUITEM_JOTTI_UPLOAD_SERVICE, L"virusscan.jotti.org", L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
    { MENUITEM_JOTTI_UPLOAD, L"virusscan.jotti.org", L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
    { MENUITEM_JOTTI_UPLOAD_SERVICE, L"virusscan.jotti.org", L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
    { MENUITEM_FILESCANIO_UPLOAD, L"www.filescan.io", L"/api/scan/file", L"file" },
    { MENUITEM_FILESCANIO_UPLOAD_SERVICE, L"www.filescan.io", L"/api/scan/file", L"file" },
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

    if (ErrorCode == STATUS_PCP_TICKET_MISSING)
    {
        PhMoveReference(&Context->ErrorString, PhCreateString(Error));
    }
    else
    {
        if (message = PhHttpGetErrorMessage(ErrorCode))
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
    }

    PostMessage(Context->DialogHandle, UM_ERROR, 0, 0);
}

CONST SERVICE_INFO* GetUploadServiceInfo(
    _In_ ULONG Id
    )
{
    for (ULONG i = 0; i < ARRAYSIZE(UploadServiceInfo); i++)
    {
        if (UploadServiceInfo[i].Id == Id)
            return &UploadServiceInfo[i];
    }

    return NULL;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID UploadContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PUPLOAD_CONTEXT context = Object;

    //if (context->TaskbarListClass)
    //{
    //    PhTaskbarListDestroy(context->TaskbarListClass);
    //    context->TaskbarListClass = NULL;
    //}

    PhClearReference(&context->ErrorString);
    PhClearReference(&context->FileName);
    PhClearReference(&context->BaseFileName);
    PhClearReference(&context->LaunchCommand);
    PhClearReference(&context->Detected);
    PhClearReference(&context->FirstAnalysisDate);
    PhClearReference(&context->LastAnalysisDate);
    PhClearReference(&context->LastAnalysisUrl);
}

VOID TaskDialogFreeContext(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    //if (Context->TaskbarListClass)
    //    PhTaskbarListSetProgressState(Context->TaskbarListClass, Context->DialogHandle, PH_TBLF_NOPROGRESS);

    PhDereferenceObject(Context);
}

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
    KPRIORITY priority;
    IO_PRIORITY_HINT ioPriority;
    BYTE buffer[PAGE_SIZE];

    if (KsiLevel() == KphLevelMax)
    {
        KPH_HASH_INFORMATION hash;

        switch (Algorithm)
        {
        case Md5HashAlgorithm:
            hash.Algorithm = KphHashAlgorithmMd5;
            break;
        case Sha1HashAlgorithm:
            hash.Algorithm = KphHashAlgorithmSha1;
            break;
        case Sha256HashAlgorithm:
            hash.Algorithm = KphHashAlgorithmSha256;
            break;
        default:
            hash.Algorithm = MaxKphHashAlgorithm;
            break;
        }

        if (hash.Algorithm != MaxKphHashAlgorithm)
        {
            status = KsiQueryHashInformationFile(FileHandle, &hash, sizeof(hash));
            if (NT_SUCCESS(status))
            {
                *HashString = PhBufferToHexString(hash.Hash, hash.Length);
                return status;
            }
        }
    }

    bytesRemaining = FileSize->QuadPart;

    PhGetThreadBasePriority(NtCurrentThread(), &priority);
    PhGetThreadIoPriority(NtCurrentThread(), &ioPriority);
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_LOWEST);
    PhSetThreadIoPriority(NtCurrentThread(), IoPriorityVeryLow);
    PhSetFileIoPriorityHint(FileHandle, IoPriorityVeryLow);

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
            hashString = PhBufferToHexString(hash, 16);
            break;
        case Sha1HashAlgorithm:
            PhFinalHash(&hashContext, hash, 20, NULL);
            hashString = PhBufferToHexString(hash, 20);
            break;
        case Sha256HashAlgorithm:
            PhFinalHash(&hashContext, hash, 32, NULL);
            hashString = PhBufferToHexString(hash, 32);
            break;
        }

        status = PhSetFilePosition(FileHandle, NULL);
    }

    PhSetThreadBasePriority(NtCurrentThread(), priority);
    PhSetThreadIoPriority(NtCurrentThread(), ioPriority);

    *HashString = hashString;

    return status;
}

PPH_BYTES PerformSubRequest(
    _In_ PUPLOAD_CONTEXT Context,
    _In_ PCWSTR HostName,
    _In_ PCWSTR ObjectName
    )
{
    NTSTATUS status;
    PPH_BYTES result = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
    {
        RaiseUploadError(Context, L"Unable to create the http socket.", status);
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, HostName, PH_HTTP_DEFAULT_HTTPS_PORT)))
    {
        RaiseUploadError(Context, L"Unable to connect to the service.", status);
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, ObjectName, PH_HTTP_FLAG_SECURE)))
    {
        RaiseUploadError(Context, L"Unable to create the request.", status);
        goto CleanupExit;
    }

    if (Context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD || Context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE)
    {
        PPH_STRING httpHeaderString;

        httpHeaderString = PhConcatStrings2(
            L"\x0061\x0070\x0069\x002D\x006B\x0065\x0079: ",
            PhGetStringOrEmpty(Context->HybridPat)
            );
        PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr);
        PhClearReference(&httpHeaderString);
    }
    else if (Context->Service == MENUITEM_VIRUSTOTAL_UPLOAD || Context->Service == MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE)
    {
        static CONST PH_STRINGREF httpHeader = PH_STRINGREF_INIT(L"accept: application/json");
        PhHttpAddRequestHeadersSR(httpContext, &httpHeader);

        PPH_STRING httpHeaderString = PhConcatStrings2(
            L"x-\x0061\x0070\x0069\x006B\x0065\x0079: ",
            PhGetStringOrEmpty(Context->TotalPat)
            );
        PhHttpAddRequestHeadersSR(httpContext, &httpHeaderString->sr);
        PhClearReference(&httpHeaderString);
    }
    else if (Context->Service == MENUITEM_FILESCANIO_UPLOAD || Context->Service == MENUITEM_FILESCANIO_UPLOAD_SERVICE)
    {
        static CONST PH_STRINGREF httpHeader = PH_STRINGREF_INIT(L"accept: application/json");
        PhHttpAddRequestHeadersSR(httpContext, &httpHeader);
        WCHAR httpHeaderText[0x40] = L"";
        volatile unsigned githubsigh[] = { 0x1e0, 0xb4, 0x184, 0x1c0, 0x1a4, 0xb4, 0x1ac, 0x194, 0x1e4 };
        for (ULONG i = 0; i < RTL_NUMBER_OF(githubsigh); i++) httpHeaderText[i] = (WCHAR)_rotr(githubsigh[i], 2);
        PPH_STRING string = PhFormatString(L"%s: %s", httpHeaderText, PhGetStringOrEmpty(Context->FileScanPat));
        PhHttpAddRequestHeadersSR(httpContext, &string->sr);
        PhClearReference(&string);
    }

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
    {
        RaiseUploadError(Context, L"Unable to send the request.", status);
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
    {
        RaiseUploadError(Context, L"Unable to receive the request.", status);
        goto CleanupExit;
    }

    if (
        Context->Service == MENUITEM_VIRUSTOTAL_UPLOAD ||
        Context->Service == MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE ||
        Context->Service == MENUITEM_JOTTI_UPLOAD ||
        Context->Service == MENUITEM_JOTTI_UPLOAD_SERVICE
        )
    {
        if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
        {
            RaiseUploadError(Context, L"Unable to receive the request.", status);
            goto CleanupExit;
        }
    }

    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &result)))
    {
        RaiseUploadError(Context, L"Unable to download the response.", status);
        goto CleanupExit;
    }

CleanupExit:

    if (httpContext)
        PhHttpDestroy(httpContext);

    return result;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UploadFileThreadStart(
    _In_ PVOID Parameter
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG httpStatus = 0;
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
    ULONG numberOfBytesRead;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpHostName = NULL;
    PPH_STRING httpHostPath = NULL;
    USHORT httpHostPort = 0;
    CONST SERVICE_INFO* serviceInfo = NULL;
    PPH_STRING postBoundary = NULL;
    PPH_BYTES asciiPostData = NULL;
    PPH_BYTES asciiFooterData = NULL;
    PH_STRING_BUILDER httpRequestHeaders = { 0 };
    PH_STRING_BUILDER httpPostHeader = { 0 };
    PH_STRING_BUILDER httpPostFooter = { 0 };

    serviceInfo = GetUploadServiceInfo(context->Service);

    if (PhIsNullOrEmptyString(context->FileUpload))
    {
        RaiseUploadError(context, L"Unable to upload the file", STATUS_FAIL_CHECK);
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhCreateFile(
        &fileHandle,
        &context->FileName->sr,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        RaiseUploadError(context, L"Unable to open the file", status);
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpCrackUrl(context->FileUpload, &httpHostName, &httpHostPath, &httpHostPort)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, PhGetString(httpHostName), httpHostPort)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", PhGetString(httpHostPath), (httpHostPort == PH_HTTP_DEFAULT_HTTPS_PORT ? PH_HTTP_FLAG_SECURE : 0))))
        goto CleanupExit;

    PhInitializeStringBuilder(&httpRequestHeaders, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostHeader, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostFooter, DOS_MAX_PATH_LENGTH);

    // HTTP request boundary string.
    {
        PH_FORMAT format[2];

        // --%I64u
        PhInitFormatS(&format[0], L"--");
        PhInitFormatI64U(&format[1], PhGenerateRandomNumber64());

        postBoundary = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }

    if (
        context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD ||
        context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE
        )
    {
        USHORT machineType;
        USHORT environmentId;
        PH_MAPPED_IMAGE mappedImage;

        if (!NT_SUCCESS(status = PhLoadMappedImageHeaderPageSize(NULL, fileHandle, &mappedImage)))
        {
            RaiseUploadError(context, L"Unable to load the image.", status);
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
        default:
            machineType = 0;
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
        PhAppendStringBuilder2(&httpRequestHeaders, L"accept: application/json\r\n");
        PhAppendStringBuilder2(&httpRequestHeaders, L"\x0061\x0070\x0069\x002D\x006B\x0065\x0079: ");
        PhAppendStringBuilder(&httpRequestHeaders, &context->HybridPat->sr);
        PhAppendStringBuilder2(&httpRequestHeaders, L"\r\n");
        PhAppendStringBuilder2(&httpRequestHeaders, L"Content-Type: multipart/form-data; boundary=");
        PhAppendStringBuilder(&httpRequestHeaders, &postBoundary->sr);
        PhAppendStringBuilder2(&httpRequestHeaders, L"\r\n");

        // POST boundary header.
        PhAppendStringBuilder2(&httpPostHeader, L"--");
        PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostHeader, L"\r\n");
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"environment_id\"\r\n\r\n%hu\r\n",
            environmentId
            );

        PhAppendStringBuilder2(&httpPostHeader, L"--");
        PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostHeader, L"\r\n");
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n\r\n",
            PhGetStringOrEmpty(context->BaseFileName)
            );

        // POST boundary footer.
        PhAppendStringBuilder2(&httpPostFooter, L"\r\n--");
        PhAppendStringBuilder(&httpPostFooter, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostFooter, L"--\r\n");
    }
    else if (
        context->Service == MENUITEM_JOTTI_UPLOAD ||
        context->Service == MENUITEM_JOTTI_UPLOAD_SERVICE
        )
    {
        PhAppendFormatStringBuilder(
            &httpRequestHeaders,
            L"Content-Type: multipart/form-data; boundary=%s\r\n",
            PhGetStringOrEmpty(postBoundary)
            );

        // POST boundary header.
        PhAppendStringBuilder2(&httpPostHeader, L"\r\n");
        PhAppendStringBuilder2(&httpPostHeader, L"--");
        PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostHeader, L"\r\n");

        PhAppendStringBuilder2(&httpPostHeader, L"Content-Disposition: form-data; name=\"MAX_FILE_SIZE\"\r\n\r\n268435456\r\n");
        PhAppendStringBuilder2(&httpPostHeader, L"--");
        PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostHeader, L"\r\n");

        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",
            serviceInfo->FileNameFieldName,
            PhGetStringOrEmpty(context->BaseFileName)
            );
        PhAppendStringBuilder2(
            &httpPostHeader,
            L"Content-Type: application/x-msdownload\r\n\r\n"
            );

        // POST boundary footer.
        PhAppendStringBuilder2(&httpPostFooter, L"\r\n--");
        PhAppendStringBuilder(&httpPostFooter, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostFooter, L"--\r\n");
    }
    else if (
        context->Service == MENUITEM_VIRUSTOTAL_UPLOAD ||
        context->Service == MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE
        )
    {
        // HTTP request headers
        PhAppendStringBuilder2(&httpRequestHeaders, L"accept: application/json\r\n");
        PhAppendStringBuilder2(&httpRequestHeaders, L"x-\x0061\x0070\x0069\x006B\x0065\x0079: ");
        PhAppendStringBuilder(&httpRequestHeaders, &context->TotalPat->sr);
        PhAppendStringBuilder2(&httpRequestHeaders, L"\r\n");
        PhAppendStringBuilder2(&httpRequestHeaders, L"Content-Type: multipart/form-data; boundary=");
        PhAppendStringBuilder(&httpRequestHeaders, &postBoundary->sr);
        PhAppendStringBuilder2(&httpRequestHeaders, L"\r\n");

        // POST boundary header
        PhAppendStringBuilder2(&httpPostHeader, L"--");
        PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostHeader, L"\r\n");

        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",
            serviceInfo->FileNameFieldName,
            PhGetStringOrEmpty(context->BaseFileName)
            );

        PhAppendStringBuilder2(
            &httpPostHeader,
            L"Content-Type: application/octet-stream\r\n\r\n"
            );

        // POST boundary footer
        PhAppendStringBuilder2(&httpPostFooter, L"\r\n--");
        PhAppendStringBuilder(&httpPostFooter, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostFooter, L"--\r\n");
    }
    else if (
        context->Service == MENUITEM_FILESCANIO_UPLOAD ||
        context->Service == MENUITEM_FILESCANIO_UPLOAD_SERVICE
        )
    {
        // HTTP request headers
        PhAppendStringBuilder2(&httpRequestHeaders, L"accept: application/json\r\n");

        PhAppendFormatStringBuilder(
            &httpRequestHeaders,
            L"Content-Type: multipart/form-data; boundary=%s\r\n",
            postBoundary->Buffer
            );

        if (!PhIsNullOrEmptyString(context->FileScanPat))
        {
            PhAppendFormatStringBuilder(
                &httpRequestHeaders,
                L"x-\x0061\x0070\x0069-\x006B\x0065\x0079: %s\r\n",
                PhGetStringOrEmpty(context->FileScanPat)
                );
        }

        // POST boundary header
        PhAppendStringBuilder2(&httpPostHeader, L"--");
        PhAppendStringBuilder(&httpPostHeader, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostHeader, L"\r\n");
        PhAppendFormatStringBuilder(
            &httpPostHeader,
            L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",
            serviceInfo->FileNameFieldName,
            PhGetStringOrEmpty(context->BaseFileName)
            );
        PhAppendStringBuilder2(&httpPostHeader, L"Content-Type: application/octet-stream\r\n\r\n");

        // POST boundary footer
        PhAppendStringBuilder2(&httpPostFooter, L"\r\n--");
        PhAppendStringBuilder(&httpPostFooter, &postBoundary->sr);
        PhAppendStringBuilder2(&httpPostFooter, L"--\r\n");
    }

    // add headers
    if (!NT_SUCCESS(status = PhHttpAddRequestHeaders(
        httpContext,
        httpRequestHeaders.String->Buffer,
        (ULONG)httpRequestHeaders.String->Length / sizeof(WCHAR)
        )))
    {
        RaiseUploadError(context, L"Unable to add request headers", status);
        goto CleanupExit;
    }

    if (context->Service == MENUITEM_JOTTI_UPLOAD)
    {
        static const PH_STRINGREF ajaxHeader = PH_STRINGREF_INIT(L"X-Requested-With: XMLHttpRequest");
        PhHttpAddRequestHeaders(httpContext, ajaxHeader.Buffer, (ULONG)ajaxHeader.Length / sizeof(WCHAR));
    }

    // Calculate the total request length.
    totalUploadLength =
        (ULONG)httpPostHeader.String->Length / sizeof(WCHAR) +
        context->TotalFileLength +
        (ULONG)httpPostFooter.String->Length / sizeof(WCHAR);

    // Send the request.
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, totalUploadLength)))
    {
        RaiseUploadError(context, L"Unable to send the request", status);
        goto CleanupExit;
    }

    // Convert to ASCII
    asciiPostData = PhConvertStringToUtf8(httpPostHeader.String);
    asciiFooterData = PhConvertStringToUtf8(httpPostFooter.String);

    // Start the clock.
    PhQuerySystemTime(&timeStart);

    // Write the header
    if (!NT_SUCCESS(status = PhHttpWriteData(
        httpContext,
        asciiPostData->Buffer,
        (ULONG)asciiPostData->Length,
        &totalPostHeaderWritten
        )))
    {
        RaiseUploadError(context, L"Unable to write the post header", status);
        goto CleanupExit;
    }

    {
        PPH_STRING msg = PhFormatString(L"Uploading %s...", PhGetStringOrEmpty(context->BaseFileName));
        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhGetString(msg));
        PhDereferenceObject(msg);
    }

    //if (context->TaskbarListClass)
    //{
    //    PhTaskbarListSetProgressState(context->TaskbarListClass, context->DialogHandle, PH_TBLF_NORMAL);
    //}

    BYTE buffer[PAGE_SIZE];

    while (TRUE)
    {
        if (context->Cancel)
        {
            RaiseUploadError(context, L"Unable to complete the request.", STATUS_CANCELLED);
            goto CleanupExit;
        }

        status = PhReadFile(
            fileHandle,
            buffer,
            PAGE_SIZE,
            NULL,
            &numberOfBytesRead
            );

        if (status == STATUS_END_OF_FILE)
            break;

        if (!NT_SUCCESS(status))
        {
            RaiseUploadError(context, L"Unable to read the file", status);
            break;
        }

        if (!NT_SUCCESS(status = PhHttpWriteData(
            httpContext,
            buffer,
            numberOfBytesRead,
            &totalWriteLength
            )))
        {
            RaiseUploadError(context, L"Unable to upload the file data", status);
            goto CleanupExit;
        }

        PhQuerySystemTime(&timeNow);

        totalUploadedLength += totalWriteLength;
        timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
        timeBitsPerSecond = totalUploadedLength / __max(timeTicks, 1);

#ifdef FORCE_NO_STATUS_TIMER
        ULONG percent = totalUploadedLength * 100 / context->TotalFileLength;
        PH_FORMAT format[9];
        WCHAR string[MAX_PATH];

        // L"Uploaded: %s / %s (%.0f%%)\r\nSpeed: %s/s"
        PhInitFormatS(&format[0], L"Uploaded: ");
        PhInitFormatSize(&format[1], totalUploadedLength);
        PhInitFormatS(&format[2], L" of ");
        PhInitFormatSize(&format[3], context->TotalFileLength);
        PhInitFormatS(&format[4], L" (");
        PhInitFormatU(&format[5], percent);
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
           PhTaskbarListSetProgressValue(context->TaskbarListClass, context->DialogHandle, totalUploadedLength, context->TotalFileLength);
       }
#else
        InterlockedExchange64(&context->ProgressTotal, context->TotalFileLength);
        InterlockedExchange64(&context->ProgressUploaded, totalUploadedLength);
        InterlockedExchange64(&context->ProgressBitsPerSecond, timeBitsPerSecond);
#endif

        //{
        //    FLOAT percent = ((FLOAT)totalUploadedLength / context->TotalFileLength * 100);
        //    PH_FORMAT format[9];
        //    WCHAR string[MAX_PATH];
        //
        //    // L"Uploaded: %s of %s (%.0f%%)\r\nSpeed: %s/s"
        //    PhInitFormatS(&format[0], L"Uploaded: ");
        //    PhInitFormatSize(&format[1], totalUploadedLength);
        //    PhInitFormatS(&format[2], L" of ");
        //    PhInitFormatSize(&format[3], context->TotalFileLength);
        //    PhInitFormatS(&format[4], L" (");
        //    PhInitFormatF(&format[5], percent, 1);
        //    PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
        //    PhInitFormatSize(&format[7], timeBitsPerSecond);
        //    PhInitFormatS(&format[8], L"/s");
        //
        //    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
        //    {
        //        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
        //    }
        //
        //    SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
        //
        //    if (context->TaskbarListClass)
        //    {
        //        PhTaskbarListSetProgressValue(context->TaskbarListClass, context->DialogHandle, totalUploadedLength, context->TotalFileLength);
        //    }
        //}
    }

    // Write the footer bytes
    if (!NT_SUCCESS(status = PhHttpWriteData(
        httpContext,
        asciiFooterData->Buffer,
        (ULONG)asciiFooterData->Length,
        &totalPostFooterWritten
        )))
    {
        RaiseUploadError(context, L"Unable to write the post footer", status);
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
    {
        RaiseUploadError(context, L"Unable to receive the response", status);
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus)))
    {
        RaiseUploadError(context, L"Unable to query http headers", status);
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
                ULONG64 errorCode;

                if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
                {
                    RaiseUploadError(context, L"Unable to download the response.", status);
                    goto CleanupExit;
                }

                if (NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
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
                        //switch (errorCode) { }

                        RaiseUploadError(context, L"Hybrid Analysis API error.", STATUS_FAIL_CHECK);
                        PhDereferenceObject(jsonString);
                        goto CleanupExit;
                    }

                    PhFreeJsonObject(jsonRootObject);
                }
                else
                {
                    RaiseUploadError(context, L"Unable to complete the request.", status);
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

                if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
                {
                    RaiseUploadError(context, L"Unable to complete the request.", status);
                    goto CleanupExit;
                }

                //if (!PhIsNullOrEmptyString(context->FileHash))
                //{
                //    PVIRUSTOTAL_FILE_REPORT fileReport;
                //
                //    if (fileReport = VirusTotalRequestFileReport(context->FileHash, context->TotalPat))
                //    {
                //        VirusTotalFreeFileReport(fileReport);
                //    }
                //}

                if (NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
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
                else                
                {
                    RaiseUploadError(context, L"Unable to complete the request.", status);
                    goto CleanupExit;
                }
                
                if (PhIsNullOrEmptyString(context->LaunchCommand))
                {
                    RaiseUploadError(context, L"Unable to complete the request.", STATUS_FAIL_CHECK);
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

                if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
                {
                    RaiseUploadError(context, L"Unable to complete the request", status);
                    goto CleanupExit;
                }

                if (NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
                {
                    PPH_STRING redirectUrl;

                    if (redirectUrl = PhGetJsonValueAsString(jsonRootObject, "redirecturl"))
                    {
                        PhMoveReference(&context->LaunchCommand, PhFormatString(L"https://virusscan.jotti.org%s", redirectUrl->Buffer));
                        PhDereferenceObject(redirectUrl);
                    }

                    PhFreeJsonObject(jsonRootObject);
                }
                else
                {
                    RaiseUploadError(context, L"Unable to parse the request", status);
                    goto CleanupExit;
                }

                PhDereferenceObject(jsonString);
            }
            break;
        case MENUITEM_FILESCANIO_UPLOAD:
        case MENUITEM_FILESCANIO_UPLOAD_SERVICE:
            {
                PPH_BYTES jsonString;
                PVOID jsonRootObject;

                if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
                {
                    RaiseUploadError(context, L"Unable to complete the request.", status);
                    goto CleanupExit;
                }

                if (NT_SUCCESS(status = PhCreateJsonParserEx(&jsonRootObject, jsonString, FALSE)))
                {
                    PPH_STRING flowIdText = PhGetJsonValueAsString(jsonRootObject, "flow_id");

                    if (!PhIsNullOrEmptyString(flowIdText))
                    {
                        PhMoveReference(&context->LaunchCommand, PhConcatStrings2(
                            L"https://www.filescan.io/uploads/",
                            PhGetString(flowIdText)
                            ));
                    }

                    PhFreeJsonObject(jsonRootObject);
                }
                else
                {
                    RaiseUploadError(context, L"Unable to complete the request.", status);
                    goto CleanupExit;
                }

                if (PhIsNullOrEmptyString(context->LaunchCommand))
                {
                    RaiseUploadError(context, L"Unable to complete the request.", STATUS_FAIL_CHECK);
                    PhDereferenceObject(jsonString);
                    goto CleanupExit;
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
        RaiseUploadError(context, L"Unable to complete the request (please try again after a few minutes)", ERROR_INVALID_DATA);
    }

CleanupExit:

    if (httpContext)
        PhHttpDestroy(httpContext);

    //if (context->TaskbarListClass)
    //    PhTaskbarListSetProgressState(context->TaskbarListClass, context->DialogHandle, PH_TBLF_NOPROGRESS);

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

LONGLONG UploadFileScanStringToTime(
    _In_ PPH_STRING Time
    )
{
    SYSTEMTIME systemtime = { 0 };
    LARGE_INTEGER time;
    PH_STRINGREF yyPart;
    PH_STRINGREF mmPartSr;
    PH_STRINGREF ddPartSr;
    PH_STRINGREF hrPartSr;
    PH_STRINGREF mnPartSr;
    PH_STRINGREF ssPartSr;
    PH_STRINGREF remainingPart;
    ULONG64 year, month, day, hour, minute, second;

    // %hu-%hu-%huT%hu:%hu:%huZ
    remainingPart = PhGetStringRef(Time);

    if (!PhSplitStringRefAtChar(&remainingPart, L'-', &mmPartSr, &remainingPart))
        return LONG64_MAX;
    if (!PhSplitStringRefAtChar(&remainingPart, L'-', &ddPartSr, &remainingPart))
        return LONG64_MAX;
    if (!PhSplitStringRefAtChar(&remainingPart, L' ', &yyPart, &remainingPart))
        return LONG64_MAX;
    if (!PhSplitStringRefAtChar(&remainingPart, L':', &hrPartSr, &remainingPart))
        return LONG64_MAX;
    if (!PhSplitStringRefAtChar(&remainingPart, L':', &mnPartSr, &remainingPart))
        return LONG64_MAX;
    if (!PhSplitStringRefAtChar(&remainingPart, L'Z', &ssPartSr, &remainingPart))
        return LONG64_MAX;

    if (!PhStringToUInt64(&yyPart, 10, &year))
        return LONG64_MAX;
    if (!PhStringToUInt64(&mmPartSr, 10, &month))
        return LONG64_MAX;
    if (!PhStringToUInt64(&ddPartSr, 10, &day))
        return LONG64_MAX;
    if (!PhStringToUInt64(&hrPartSr, 10, &hour))
        return LONG64_MAX;
    if (!PhStringToUInt64(&mnPartSr, 10, &minute))
        return LONG64_MAX;
    if (!PhStringToUInt64(&ssPartSr, 10, &second))
        return LONG64_MAX;

    if (!NT_SUCCESS(RtlULong64ToUShort(year, &systemtime.wYear)))
        return LONG64_MAX;
    if (!NT_SUCCESS(RtlULong64ToUShort(month, &systemtime.wYear)))
        return LONG64_MAX;
    if (!NT_SUCCESS(RtlULong64ToUShort(day, &systemtime.wYear)))
        return LONG64_MAX;
    if (!NT_SUCCESS(RtlULong64ToUShort(hour, &systemtime.wYear)))
        return LONG64_MAX;
    if (!NT_SUCCESS(RtlULong64ToUShort(minute, &systemtime.wYear)))
        return LONG64_MAX;
    if (!NT_SUCCESS(RtlULong64ToUShort(second, &systemtime.wYear)))
        return LONG64_MAX;
    if (!PhSystemTimeToLargeInteger(&time, &systemtime))
        return LONG64_MAX;

    return time.QuadPart;
}

typedef struct _PFILESCANIO_REPORT
{
    LONGLONG report_time;
    PPH_STRING report_date;
    PPH_STRING report_id;
    PPH_STRING flow_id;
} FILESCANIO_REPORT, *PFILESCANIO_REPORT;

#if defined(FILESCANIO_PROMPT)
static int __cdecl OnlineChecksFileScanIoCompareFunction(
    _In_ const void* elem1,
    _In_ const void* elem2
    )
{
    PFILESCANIO_REPORT node1 = *(PFILESCANIO_REPORT*)elem1;
    PFILESCANIO_REPORT node2 = *(PFILESCANIO_REPORT*)elem2;

    return int64cmp(node1->report_time, node2->report_time);
}
#endif

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UploadCheckThreadStart(
    _In_ PVOID Parameter
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    NTSTATUS status;
    LARGE_INTEGER fileSize64;
    PPH_BYTES subRequestBuffer = NULL;
    CONST SERVICE_INFO* serviceInfo = NULL;
    PPH_STRING subObjectName = NULL;
    HANDLE fileHandle = NULL;

    //context->Extension = VirusTotalGetCachedResult(context->FileName);
    serviceInfo = GetUploadServiceInfo(context->Service);

    if (!NT_SUCCESS(status = PhCreateFile(
        &fileHandle,
        &context->FileName->sr,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        RaiseUploadError(context, L"Unable to open the file", status);
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
        else if (
            context->Service == MENUITEM_FILESCANIO_UPLOAD ||
            context->Service == MENUITEM_FILESCANIO_UPLOAD_SERVICE
            )
        {
            if (fileSize64.QuadPart > 100 * 1024 * 1024) // 128 MB
            {
                RaiseUploadError(context, L"The file is too large (over 100 MB)", ERROR_FILE_TOO_LARGE);
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
            PVOID rootJsonObject;

            if (PhIsNullOrEmptyString(context->HybridPat))
            {
                RaiseUploadError(context, L"You need to configure HybridAnalysis from the Options window > OnlineChecks page.", STATUS_PCP_TICKET_MISSING);
                goto CleanupExit;
            }

            // Create the default upload URL.
            context->FileUpload = PhFormatString(
                L"https://%s%s",
                serviceInfo->HostName,
                serviceInfo->UploadObjectName
                );

            if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, Sha256HashAlgorithm, &tempHashString)))
            {
                RaiseUploadError(context, L"Unable to hash the file", status);
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

            if (NT_SUCCESS(status = PhCreateJsonParserEx(&rootJsonObject, subRequestBuffer, FALSE)))
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
                RaiseUploadError(context, L"Unable to parse the response.", status);
            }
        }
        break;
    case MENUITEM_VIRUSTOTAL_UPLOAD:
    case MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE:
        {
            BOOLEAN upload = FALSE;
            PPH_STRING tempHashString = NULL;
            PVOID rootJsonObject;

            if (PhIsNullOrEmptyString(context->TotalPat))
            {
                RaiseUploadError(context, L"You need to configure VirusTotal from the Options window > OnlineChecks page.", STATUS_PCP_TICKET_MISSING);
                goto CleanupExit;
            }

            if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, Sha256HashAlgorithm, &tempHashString)))
            {
                RaiseUploadError(context, L"Unable to hash the file", status);
                goto CleanupExit;
            }

            context->FileHash = tempHashString;
            subObjectName = PhConcatStrings2(L"/api/v3/files/", PhGetString(context->FileHash));

            if (!(subRequestBuffer = PerformSubRequest(
                context,
                serviceInfo->HostName,
                subObjectName->Buffer
                )))
            {
                goto CleanupExit;
            }

            if (NT_SUCCESS(status = PhCreateJsonParserEx(&rootJsonObject, subRequestBuffer, FALSE)))
            {
                PVOID dataObject;
                PVOID attributesObject;
                PVOID statsObject;

                if (dataObject = PhGetJsonObject(rootJsonObject, "error"))
                {
                    //PhGetJsonValueAsString(dataObject, "code");
                    //PhGetJsonValueAsString(dataObject, "message");
                    upload = TRUE;
                }
                else
                {
                    if (dataObject = PhGetJsonObject(rootJsonObject, "data"))
                    {
                        if (attributesObject = PhGetJsonObject(dataObject, "attributes"))
                        {
                            if (statsObject = PhGetJsonObject(attributesObject, "last_analysis_stats"))
                            {
                                context->Detected = PhFormatUInt64(PhGetJsonValueAsUInt64(statsObject, "malicious"), FALSE);
                            }

                            context->FirstAnalysisDate = VirusTotalDateToTime((ULONG)PhGetJsonValueAsUInt64(attributesObject, "first_submission_date"));
                            context->LastAnalysisDate = VirusTotalDateToTime((ULONG)PhGetJsonValueAsUInt64(attributesObject, "last_analysis_date")); // last_submission_date
                        }
                    }
                }

                //if (!PhIsNullOrEmptyString(context->ReAnalyseUrl))
                //{
                //    PhMoveReference(&context->ReAnalyseUrl, PhFormatString(
                //        L"%s%s",
                //        L"https://www.virustotal.com",
                //        PhGetString(context->ReAnalyseUrl)
                //        ));
                //}
                //
                if (context->VtApiUpload)
                {
                    PhMoveReference(&context->FileUpload, PhCreateString(L"https://www.virustotal.com/api/v3/files"));
                }
                else
                {
                    PPH_STRING vt3UploadUrl;
                    PPH_BYTES vt3UploadRequestBuffer;
                    PVOID vt3RootJsonObject;

                    vt3UploadUrl = PhCreateString(L"/api/v3/files/upload_url");

                    if (!(vt3UploadRequestBuffer = PerformSubRequest(
                        context,
                        serviceInfo->HostName,
                        vt3UploadUrl->Buffer
                        )))
                    {
                        PhDereferenceObject(vt3UploadUrl);
                        goto CleanupExit;
                    }
                    PhDereferenceObject(vt3UploadUrl);

                    if (NT_SUCCESS(status = PhCreateJsonParserEx(&vt3RootJsonObject, vt3UploadRequestBuffer, FALSE)))
                    {
                        PhMoveReference(&context->FileUpload, PhGetJsonValueAsString(vt3RootJsonObject, "data"));
                        PhFreeJsonObject(vt3RootJsonObject);
                    }
                    else
                    {
                        RaiseUploadError(context, L"Unable to parse the response.", status);
                    }
                    
                    PhClearReference(&vt3UploadRequestBuffer);
                }

                PhMoveReference(&context->LaunchCommand, PhFormatString(
                    L"https://www.virustotal.com/file/%s/analysis/",
                    PhGetString(context->FileHash)
                    ));

                if (upload)
                    PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
                else
                    PostMessage(context->DialogHandle, UM_EXISTS, 0, 0);

                PhFreeJsonObject(rootJsonObject);
            }
            else
            {
                RaiseUploadError(context, L"Unable to parse the response.", status);
            }
        }
        break;
    case MENUITEM_JOTTI_UPLOAD:
    case MENUITEM_JOTTI_UPLOAD_SERVICE:
        {
            // Create the default upload URL.
            context->FileUpload = PhFormatString(L"https://%s%s", serviceInfo->HostName, serviceInfo->UploadObjectName);

            // Start the upload.
            PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
        }
        break;
    case MENUITEM_FILESCANIO_UPLOAD:
    case MENUITEM_FILESCANIO_UPLOAD_SERVICE:
        {
            PPH_STRING tempHashString = NULL;
            PVOID rootJsonObject;

            // Create the default upload URL.
            context->FileUpload = PhFormatString(
                L"https://%s%s",
                serviceInfo->HostName,
                serviceInfo->UploadObjectName
                );

            if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, Sha256HashAlgorithm, &tempHashString)))
            {
                RaiseUploadError(context, L"Unable to hash the file", status);
                goto CleanupExit;
            }

            context->FileHash = tempHashString;
            subObjectName = PhConcatStrings2(L"/api/reputation/hash?sha256=", PhGetString(context->FileHash));

            if (!(subRequestBuffer = PerformSubRequest(
                context,
                serviceInfo->HostName,
                subObjectName->Buffer
                )))
            {
                goto CleanupExit;
            }

            if (NT_SUCCESS(status = PhCreateJsonParserEx(&rootJsonObject, subRequestBuffer, FALSE)))
            {
                #if defined(FILESCANIO_PROMPT)
                PVOID jsonDataObject;
                LONG reportsCount;
                
                if (jsonDataObject = PhGetJsonObject(rootJsonObject, "filescan_reports"))
                {
                    if (reportsCount = PhGetJsonArrayLength(jsonDataObject))
                    {
                        PPH_LIST reportList = PhCreateList(reportsCount);
                
                        for (LONG i = 0; i < reportsCount; i++)
                        {
                            PVOID report;
                            PFILESCANIO_REPORT scan_report;
                
                            report = PhGetJsonArrayIndexObject(jsonDataObject, i);
                
                            scan_report = PhAllocate(sizeof(FILESCANIO_REPORT));
                            scan_report->report_date = PhGetJsonValueAsString(report, "report_date");
                            scan_report->report_id = PhGetJsonValueAsString(report, "report_id");
                            scan_report->flow_id = PhGetJsonValueAsString(report, "flow_id");
                            scan_report->report_time = UploadFileScanStringToTime(scan_report->report_date);
                            PhAddItemList(reportList, scan_report);
                        }
                
                        qsort(reportList->Items, reportList->Count, sizeof(PVOID), OnlineChecksFileScanIoCompareFunction);
                
                        PFILESCANIO_REPORT latest_report = reportList->Items[reportList->Count - 1];
                
                        context->LastAnalysisDate = latest_report->report_date;
                
                        if (jsonDataObject = PhGetJsonObject(rootJsonObject, "mdcloud"))
                        {
                            PhMoveReference(&context->Detected, PhFormatString(
                                L"%lu/%lu",
                                PhGetJsonValueAsUlong(jsonDataObject, "detected_av_engines"),
                                PhGetJsonValueAsUlong(jsonDataObject, "total_av_engines")
                                ));
                        }
                
                        PhMoveReference(&context->LaunchCommand, PhFormatString(
                            L"https://www.filescan.io/uploads/%s/reports/%s/overview",
                            PhGetString(latest_report->flow_id),
                            PhGetString(latest_report->report_id
                            )));
                        //PhMoveReference(&context->LaunchCommand, PhFormatString(
                        //    L"https://www.filescan.io/uploads/%s",
                        //    PhGetString(latest_report->flow_id)
                        //    ));
                
                        //PostMessage(context->DialogHandle, UM_LAUNCH, 0, 0);
                        PostMessage(context->DialogHandle, UM_EXISTS, 0, 0);
                    }
                    else
                    {
                        PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
                    }
                }
                else
                #endif
                {
                    PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
                }

                PhFreeJsonObject(rootJsonObject);
            }
            else
            {
                RaiseUploadError(context, L"Unable to parse the response.", status);
            }
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

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UploadRecheckThreadStart(
    _In_ PVOID Parameter
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    PVIRUSTOTAL_API_RESPONSE fileRescan;

    if (fileRescan = VirusTotalRequestFileReScan(context->FileHash, context->TotalPat))
    {
        if (fileRescan->ResponseCode == 1)
        {
            //PhSwapReference(&context->ReAnalyseUrl, fileRescan->PermaLink);
            //PhShellExecute(NULL, PhGetString(context->ReAnalyseUrl), NULL);

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
        RaiseUploadError(context, L"VirusTotal ReScan API error.", STATUS_FAIL_CHECK);
    }

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ViewReportThreadStart(
    _In_ PVOID Parameter
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    PVIRUSTOTAL_FILE_REPORT fileReport;

    if (fileReport = VirusTotalRequestFileReport(context->FileHash, context->TotalPat))
    {
        PhSwapReference(&context->LaunchCommand, PhConcatStrings2(L"https://www.virustotal.com/gui/file/", PhGetString(context->FileHash)));

        PhShellExecute(NULL, PhGetString(context->LaunchCommand), NULL);

        SendMessage(context->DialogHandle, TDM_CLICK_BUTTON, IDOK, 0);

        VirusTotalFreeFileReport(fileReport);
    }
    else
    {
        RaiseUploadError(context, L"VirusTotal ViewReport API error.", STATUS_FAIL_CHECK);
    }

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

LRESULT CALLBACK OnlineChecksTaskDialogSubclass(
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
#ifndef FORCE_NO_STATUS_TIMER
            if (context->ProgressTimer)
            {
                PhKillTimer(hwndDlg, 9000);
                context->ProgressTimer = FALSE;
            }
#endif
            PhSetWindowProcedure(hwndDlg, oldWndProc);
            PhRemoveWindowContext(hwndDlg, 0xF);

            TaskDialogFreeContext(context);
        }
        break;
    case UM_UPLOAD:
        {
            ShowFileUploadProgressDialog(context);
        }
        break;
    case UM_EXISTS:
        {
#ifndef FORCE_NO_STATUS_TIMER
            if (context->ProgressTimer)
            {
                PhKillTimer(hwndDlg, 9000);
                context->ProgressTimer = FALSE;
            }
#endif
            if (
                context->Service == MENUITEM_VIRUSTOTAL_UPLOAD ||
                context->Service == MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE
                )
            {
                switch (PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_ACTION))
                {
                case 1:
                    ShowFileUploadProgressDialog(context);
                    break;
                case 2:
                    ShowVirusTotalReScanProgressDialog(context);
                    break;
                case 3:
                    ShowVirusTotalViewReportProgressDialog(context);
                    break;
                default:
                    ShowFileFoundDialog(context);
                    break;
                }
            }
            else if (
                context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD ||
                context->Service == MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE
                )
            {
                ShowFileFoundDialog(context);
            }
            else if (
                context->Service == MENUITEM_FILESCANIO_UPLOAD ||
                context->Service == MENUITEM_FILESCANIO_UPLOAD_SERVICE
                )
            {
                ShowFileFoundDialog(context);
            }
        }
        break;
    case UM_LAUNCH:
        {
#ifndef FORCE_NO_STATUS_TIMER
            if (context->ProgressTimer)
            {
                PhKillTimer(hwndDlg, 9000);
                context->ProgressTimer = FALSE;
            }
#endif
            if (!PhIsNullOrEmptyString(context->LaunchCommand))
            {
                PhShellExecute(hwndDlg, context->LaunchCommand->Buffer, NULL);
            }

            SendMessage(hwndDlg, TDM_CLICK_BUTTON, IDOK, 0);
        }
        break;
    case UM_ERROR:
        {
#ifndef FORCE_NO_STATUS_TIMER
            if (context->ProgressTimer)
            {
                PhKillTimer(hwndDlg, 9000);
                context->ProgressTimer = FALSE;
            }
#endif
            VirusTotalShowErrorDialog(context);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == 9000)
            {
                if (context->ProgressUploaded && context->ProgressTotal)
                {
                    LONG64 percent = context->ProgressUploaded * 100 / context->ProgressTotal;
                    PH_FORMAT format[9];
                    WCHAR string[MAX_PATH];

                    // L"Uploaded: %s / %s (%.0f%%)\r\nSpeed: %s/s"
                    PhInitFormatS(&format[0], L"Uploaded: ");
                    PhInitFormatSize(&format[1], context->ProgressUploaded);
                    PhInitFormatS(&format[2], L" of ");
                    PhInitFormatSize(&format[3], context->ProgressTotal);
                    PhInitFormatS(&format[4], L" (");
                    PhInitFormatI64U(&format[5], percent);
                    PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
                    PhInitFormatSize(&format[7], context->ProgressBitsPerSecond);
                    PhInitFormatS(&format[8], L"/s");

                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                    {
                        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                    }

                    if (context->ProgressMarquee)
                    {
                        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);

                        //if (context->TaskbarListClass)
                        //{
                        //    PhTaskbarListSetProgressState(context->TaskbarListClass, context->DialogHandle, PH_TBLF_NOPROGRESS);
                        //}

                        context->ProgressMarquee = FALSE;
                    }

                    SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);

                    //if (context->TaskbarListClass)
                    //{
                    //    PhTaskbarListSetProgressValue(context->TaskbarListClass, context->DialogHandle, context->ProgressUploaded, context->TotalFileLength);
                    //}
                }
                return 0;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

HRESULT CALLBACK OnlineChecksTaskDialogBootstrap(
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
            HWND windowHandle = SystemInformer_GetWindowHandle();

            context->DialogHandle = hwndDlg;
            context->HybridPat = PhGetStringSetting(SETTING_NAME_HYBRIDANAL_DEFAULT_PAT);
            context->TotalPat = PhGetStringSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT);
            context->FileScanPat = PhGetStringSetting(SETTING_NAME_FILESCAN_DEFAULT_PAT);

            if (PhIsNullOrEmptyString(context->HybridPat))
            {
                context->HybridPat = PhConcatStrings(6,
                    L"4kc4k0ck8c48k", L"gkoccosc8o0",
                    L"g0g0cccggg8", L"4gg0cw8ssw8",
                    L"ogk0cw0g0cw", L"4s00ccs"
                    );
            }

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(windowHandle) && !IsMinimized(windowHandle)) ? windowHandle : NULL);

            // Create the Taskdialog icons
            PhSetApplicationWindowIcon(hwndDlg);

            //PhTaskbarListCreate(&context->TaskbarListClass);

            context->DialogWindowProc = PhGetWindowProcedure(hwndDlg);
            PhSetWindowContext(hwndDlg, 0xF, context);
            PhSetWindowProcedure(hwndDlg, OnlineChecksTaskDialogSubclass);

            ShowFileUploadDialog(context);
        }
        break;
    }

    return S_OK;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS OnlineChecksUploadDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    PhInitializeAutoPool(&autoPool);

    config.hInstance = PluginInstance->DllBase;
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = OnlineChecksTaskDialogBootstrap;
    PhShowTaskDialog(&config, NULL, NULL, NULL);

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

    if (PhDetermineDosPathNameType(&FileName->sr) == RtlPathTypeDriveAbsolute)
    {
        PPH_STRING fileNtPathName;

        if (fileNtPathName = PhDosPathNameToNtPathName(&FileName->sr))
        {
            context = PhCreateObjectZero(sizeof(UPLOAD_CONTEXT), UploadContextType);
            context->Service = Service;
            context->FileName = fileNtPathName;
            context->BaseFileName = PhGetBaseName(context->FileName);

            PhCreateThread2(OnlineChecksUploadDialogThread, context);
        }
    }
    else
    {
        PhReferenceObject(FileName);

        context = PhCreateObjectZero(sizeof(UPLOAD_CONTEXT), UploadContextType);
        context->Service = Service;
        context->FileName = FileName;
        context->BaseFileName = PhGetBaseName(context->FileName);

        PhCreateThread2(OnlineChecksUploadDialogThread, context);
    }
}

VOID UploadServiceToOnlineService(
    _In_opt_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG Service
    )
{
    if (PhBeginInitOnce(&UploadContextTypeInitOnce))
    {
        UploadContextType = PhCreateObjectType(L"OnlineChecksObjectType", 0, UploadContextDeleteProcedure);
        PhEndInitOnce(&UploadContextTypeInitOnce);
    }

    if (ServiceItem->FileName)
    {
        if (PhDetermineDosPathNameType(&ServiceItem->FileName->sr) == RtlPathTypeDriveAbsolute)
        {
            PPH_STRING fileNtPathName;

            if (fileNtPathName = PhDosPathNameToNtPathName(&ServiceItem->FileName->sr))
            {
                PUPLOAD_CONTEXT context;

                context = PhCreateObjectZero(sizeof(UPLOAD_CONTEXT), UploadContextType);
                context->Service = Service;
                context->FileName = fileNtPathName;
                context->BaseFileName = PhGetBaseName(context->FileName);

                PhCreateThread2(OnlineChecksUploadDialogThread, context);
            }
        }
        else
        {
            PUPLOAD_CONTEXT context;

            context = PhCreateObjectZero(sizeof(UPLOAD_CONTEXT), UploadContextType);
            context->Service = Service;
            context->FileName = ServiceItem->FileName;
            context->BaseFileName = PhGetBaseName(context->FileName);

            PhCreateThread2(OnlineChecksUploadDialogThread, context);
        }
    }
    else
    {
        PhShowStatus(WindowHandle, L"Unable to query the service.", STATUS_OBJECT_NAME_NOT_FOUND, 0);
    }
}
