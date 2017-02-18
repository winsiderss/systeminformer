/*
 * Process Hacker Plugins -
 *   Online Checks Plugin
 *
 * Copyright (C) 2016 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "onlnchk.h"
#include <commonutil.h>
#include <shlobj.h>

PPH_OBJECT_TYPE UploadContextType = NULL;
SERVICE_INFO UploadServiceInfo[] =
{
    { MENUITEM_VIRUSTOTAL_UPLOAD, L"www.virustotal.com", INTERNET_DEFAULT_HTTPS_PORT, WINHTTP_FLAG_SECURE, L"???", L"file" },
    { MENUITEM_JOTTI_UPLOAD, L"virusscan.jotti.org", INTERNET_DEFAULT_HTTPS_PORT, WINHTTP_FLAG_SECURE, L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
};

BOOL ReadRequestString(
    _In_ HINTERNET Handle,
    _Out_ _Deref_post_z_cap_(*DataLength) PSTR *Data,
    _Out_ ULONG *DataLength
    )
{
    BYTE buffer[PAGE_SIZE];
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    // Zero the buffer
    memset(buffer, 0, PAGE_SIZE);

    while (WinHttpReadData(Handle, buffer, PAGE_SIZE, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        // Copy the returned buffer into our pointer
        memcpy(data + dataLength, buffer, returnLength);
        // Zero the returned buffer for the next loop
        //memset(buffer, 0, returnLength);

        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    *DataLength = dataLength;
    *Data = data;

    return TRUE;
}

VOID RaiseUploadError(
    _In_ PUPLOAD_CONTEXT Context,
    _In_ PWSTR Error,
    _In_ ULONG ErrorCode
    )
{
    PWSTR errorMessage = NULL;
    PPH_STRING message;

    if (!Context->DialogHandle)
        return;

    if (message = PhGetMessage(GetModuleHandle(L"winhttp.dll"), 0xb, GetUserDefaultLangID(), ErrorCode))
    {
        PhTrimToNullTerminatorString(message);
    }

    // Remove any trailing newline
    if (message && message->Length >= 2 * sizeof(WCHAR) &&
        message->Buffer[message->Length / sizeof(WCHAR) - 2] == '\r' &&
        message->Buffer[message->Length / sizeof(WCHAR) - 1] == '\n')
    {
        PhMoveReference(&message, PhCreateStringEx(message->Buffer, message->Length - 2 * sizeof(WCHAR)));
    }

    if (message)
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

    if (context->UploadThreadHandle)
    {
        NtClose(context->UploadThreadHandle);
        context->UploadThreadHandle = NULL;
    }

    if (context->HttpHandle)
    {
        WinHttpCloseHandle(context->HttpHandle);
        context->HttpHandle = NULL;
    }

    if (context->IconLargeHandle)
    {
        DestroyIcon(context->IconLargeHandle);
        context->IconLargeHandle = NULL;
    }

    if (context->IconSmallHandle)
    {
        DestroyIcon(context->IconSmallHandle);
        context->IconSmallHandle = NULL;
    }

    PhClearReference(&context->ErrorString);
    PhClearReference(&context->KeyString);
    PhClearReference(&context->FileName);
    PhClearReference(&context->BaseFileName);
    PhClearReference(&context->WindowFileName);
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

VOID TaskDialogCreateIcons(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    Context->IconLargeHandle = PhLoadIcon(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        PH_LOAD_ICON_SIZE_LARGE,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON)
        );
    Context->IconSmallHandle = PhLoadIcon(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        PH_LOAD_ICON_SIZE_LARGE,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON)
        );

    SendMessage(Context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)Context->IconSmallHandle);
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)Context->IconLargeHandle);
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

NTSTATUS HashFileAndResetPosition(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER FileSize,
    _In_ PH_HASH_ALGORITHM Algorithm,
    _Out_ PPH_STRING *HashString
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PH_HASH_CONTEXT hashContext;
    PPH_STRING hashString = NULL;
    ULONG64 bytesRemaining;
    FILE_POSITION_INFORMATION positionInfo;
    LONG priority;
    LONG newpriority;
    IO_PRIORITY_HINT ioPriority;
    IO_PRIORITY_HINT newioPriority;
    UCHAR buffer[PAGE_SIZE];
    
    bytesRemaining = FileSize->QuadPart;

    newpriority = THREAD_PRIORITY_LOWEST;
    newioPriority = IoPriorityVeryLow;
    NtQueryInformationThread(NtCurrentThread(), ThreadBasePriority, &priority, sizeof(LONG), NULL);
    NtQueryInformationThread(NtCurrentThread(), ThreadIoPriority, &ioPriority, sizeof(IO_PRIORITY_HINT), NULL);
    NtSetInformationThread(NtCurrentThread(), ThreadBasePriority, &newpriority, sizeof(LONG));
    NtSetInformationThread(NtCurrentThread(), ThreadIoPriority, &newioPriority, sizeof(IO_PRIORITY_HINT));

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
        UCHAR hash[32];

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

        positionInfo.CurrentByteOffset.QuadPart = 0;
        status = NtSetInformationFile(
            FileHandle,
            &iosb,
            &positionInfo,
            sizeof(FILE_POSITION_INFORMATION),
            FilePositionInformation
            );
    }

    NtSetInformationThread(NtCurrentThread(), ThreadBasePriority, &priority, sizeof(LONG));
    NtSetInformationThread(NtCurrentThread(), ThreadIoPriority, &ioPriority, sizeof(IO_PRIORITY_HINT));

    return status;
}

PPH_BYTES PerformSubRequest(
    _In_ PUPLOAD_CONTEXT Context,
    _In_ PWSTR HostName,
    _In_ INTERNET_PORT HostPort,
    _In_ ULONG HostFlags,
    _In_ PWSTR ObjectName
    )
{
    PPH_BYTES result = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;

    if (!(connectHandle = WinHttpConnect(
        Context->HttpHandle,
        HostName,
        HostPort,
        0
        )))
    {
        RaiseUploadError(Context, L"Unable to connect to the service", GetLastError());
        goto CleanupExit;
    }

    if (!(requestHandle = WinHttpOpenRequest(
        connectHandle,
        NULL,
        ObjectName,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | HostFlags
        )))
    {
        RaiseUploadError(Context, L"Unable to create the request", GetLastError());
        goto CleanupExit;
    }

    if (!WinHttpSendRequest(
        requestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
        0
        ))
    {
        RaiseUploadError(Context, L"Unable to send the request", GetLastError());
        goto CleanupExit;
    }

    if (WinHttpReceiveResponse(requestHandle, NULL))
    {
        PSTR data;
        ULONG allocatedLength;
        ULONG dataLength;
        ULONG returnLength;
        BYTE buffer[PAGE_SIZE];

        allocatedLength = sizeof(buffer);
        data = PhAllocate(allocatedLength);
        dataLength = 0;

        while (WinHttpReadData(requestHandle, buffer, PAGE_SIZE, &returnLength))
        {
            if (returnLength == 0)
                break;

            if (allocatedLength < dataLength + returnLength)
            {
                allocatedLength *= 2;
                data = PhReAllocate(data, allocatedLength);
            }

            memcpy(data + dataLength, buffer, returnLength);
            dataLength += returnLength;
        }

        if (allocatedLength < dataLength + 1)
        {
            allocatedLength++;
            data = PhReAllocate(data, allocatedLength);
        }

        data[dataLength] = 0;

        result = PhCreateBytesEx(data, dataLength);
    }
    else
    {
        RaiseUploadError(Context, L"Unable to receive the response", GetLastError());
        goto CleanupExit;
    }

CleanupExit:

    if (requestHandle)
        WinHttpCloseHandle(requestHandle);

    if (connectHandle)
        WinHttpCloseHandle(connectHandle);

    return result;
}

NTSTATUS UploadFileThreadStart(
    _In_ PVOID Parameter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG httpStatus = 0;
    ULONG httpStatusLength = sizeof(ULONG);
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
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;
    IO_STATUS_BLOCK isb;
    URL_COMPONENTS httpComponents = { sizeof(URL_COMPONENTS) };
    PPH_STRING httpHostName = NULL;
    PPH_STRING httpHostPath = NULL;
    PSERVICE_INFO serviceInfo = NULL;
    PPH_STRING postBoundary = NULL;
    PPH_BYTES asciiPostData = NULL;
    PPH_BYTES asciiFooterData = NULL;
    PH_STRING_BUILDER httpRequestHeaders = { 0 };
    PH_STRING_BUILDER httpPostHeader = { 0 };
    PH_STRING_BUILDER httpPostFooter = { 0 };
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    serviceInfo = GetUploadServiceInfo(context->Service);

    // Set lengths to non-zero enabling these params to be cracked.
    httpComponents.dwSchemeLength = (ULONG)-1;
    httpComponents.dwHostNameLength = (ULONG)-1;
    httpComponents.dwUrlPathLength = (ULONG)-1;

    if (!WinHttpCrackUrl(
        PhGetStringOrEmpty(context->UploadUrl),
        0,
        0,
        &httpComponents
        ))
    {
        goto CleanupExit;
    }

    // Create the Host string.
    if (PhIsNullOrEmptyString(httpHostName = PhCreateStringEx(
        httpComponents.lpszHostName,
        httpComponents.dwHostNameLength * sizeof(WCHAR)
        )))
    {
        goto CleanupExit;
    }

    // Create the Path string.
    if (PhIsNullOrEmptyString(httpHostPath = PhCreateStringEx(
        httpComponents.lpszUrlPath,
        httpComponents.dwUrlPathLength * sizeof(WCHAR)
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        PhGetStringOrEmpty(context->FileName),
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        RaiseUploadError(context, L"Unable to open the file", RtlNtStatusToDosError(status));
        goto CleanupExit;
    }

    if (!(connectHandle = WinHttpConnect(
        context->HttpHandle,
        serviceInfo->HostName,
        serviceInfo->HostPort,
        0
        )))
    {
        RaiseUploadError(context, L"Unable to connect to the service", GetLastError());
        goto CleanupExit;
    }

    if (!(requestHandle = WinHttpOpenRequest(
        connectHandle,
        L"POST",
        PhGetStringOrEmpty(httpHostPath),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | serviceInfo->HostFlags
        )))
    {
        RaiseUploadError(context, L"Unable to create the request", GetLastError());
        goto CleanupExit;
    }

    PhInitializeStringBuilder(&httpRequestHeaders, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostHeader, DOS_MAX_PATH_LENGTH);
    PhInitializeStringBuilder(&httpPostFooter, DOS_MAX_PATH_LENGTH);

    // build request boundary string
    postBoundary = PhFormatString(
        L"------------------------%I64u",
        (ULONG64)RtlRandomEx(&httpPostSeed) | ((ULONG64)RtlRandomEx(&httpPostSeed) << 31)
        );

    // build request header string
    PhAppendFormatStringBuilder(
        &httpRequestHeaders,
        L"Content-Type: multipart/form-data; boundary=%s\r\n",
        postBoundary->Buffer
        );

    if (context->Service == MENUITEM_JOTTI_UPLOAD)
    {
        // POST boundary header
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

        // POST boundary footer
        PhAppendFormatStringBuilder(
            &httpPostFooter,
            L"\r\n--%s--\r\n",
            postBoundary->Buffer
            );
    }
    else
    {
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
    if (!WinHttpAddRequestHeaders(
        requestHandle,
        httpRequestHeaders.String->Buffer,
        -1L,
        WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD
        ))
    {
        RaiseUploadError(context, L"Unable to add request headers", GetLastError());
        goto CleanupExit;
    }

    if (context->Service == MENUITEM_JOTTI_UPLOAD)
    {
        PPH_STRING ajaxHeader = PhCreateString(L"X-Requested-With: XMLHttpRequest");

        WinHttpAddRequestHeaders(
            requestHandle,
            ajaxHeader->Buffer,
            (ULONG)ajaxHeader->Length / sizeof(WCHAR),
            WINHTTP_ADDREQ_FLAG_ADD
            );

        PhDereferenceObject(ajaxHeader);
    }

    // Calculate the total request length.
    totalUploadLength = (ULONG)httpPostHeader.String->Length / sizeof(WCHAR) + context->TotalFileLength + (ULONG)httpPostFooter.String->Length / sizeof(WCHAR);

    // Send the request.
    if (!WinHttpSendRequest(
        requestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        totalUploadLength,
        0
        ))
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
    if (!WinHttpWriteData(
        requestHandle,
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

        if (!context->UploadThreadHandle)
            goto CleanupExit;

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

        if (!WinHttpWriteData(requestHandle, buffer, (ULONG)isb.Information, &totalWriteLength))
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
            PPH_STRING totalLength = PhFormatSize(context->TotalFileLength, -1);
            PPH_STRING totalUploaded = PhFormatSize(totalUploadedLength, -1);
            PPH_STRING totalSpeed = PhFormatSize(timeBitsPerSecond, -1);
            PPH_STRING statusMessage = PhFormatString(
                L"Uploaded: %s of %s (%.0f%%)\r\nSpeed: %s/s",
                PhGetStringOrEmpty(totalUploaded),
                PhGetStringOrEmpty(totalLength),
                percent,
                PhGetStringOrEmpty(totalSpeed)
                );

            SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)statusMessage->Buffer);
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

            PhDereferenceObject(statusMessage);
            PhDereferenceObject(totalSpeed);
            PhDereferenceObject(totalUploaded);
            PhDereferenceObject(totalLength);
        }
    }

    // Write the footer bytes
    if (!WinHttpWriteData(
        requestHandle,
        asciiFooterData->Buffer,
        (ULONG)asciiFooterData->Length,
        &totalPostFooterWritten
        ))
    {
        RaiseUploadError(context, L"Unable to write the post footer", GetLastError());
        goto CleanupExit;
    }

    if (!WinHttpReceiveResponse(requestHandle, NULL))
    {
        RaiseUploadError(context, L"Unable to receive the response", GetLastError());
        goto CleanupExit;
    }

    if (!WinHttpQueryHeaders(
        requestHandle,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &httpStatus,
        &httpStatusLength,
        NULL
        ))
    {
        RaiseUploadError(context, L"Unable to query http headers", GetLastError());
        goto CleanupExit;
    }

    if (httpStatus == HTTP_STATUS_OK || httpStatus == HTTP_STATUS_REDIRECT_METHOD || httpStatus == HTTP_STATUS_REDIRECT)
    {
        switch (context->Service)
        {
        case MENUITEM_VIRUSTOTAL_UPLOAD:
            {
                PSTR buffer = NULL;
                PSTR redirectUrl;
                ULONG bufferLength;
                PVOID jsonRootObject;

                if (!ReadRequestString(requestHandle, &buffer, &bufferLength))
                {
                    RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                    goto CleanupExit;
                }

                if (jsonRootObject = CreateJsonParser(buffer))
                {
                    if (!GetJsonValueAsUlong(jsonRootObject, "response_code"))
                        goto CleanupExit;

                    // PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "resource"));
                    // PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "scan_id"));
                    // PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "sha256"));
                    // PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "verbose_msg"));

                    if (redirectUrl = GetJsonValueAsString(jsonRootObject, "permalink"))
                    {
                        PhMoveReference(&context->LaunchCommand, PhZeroExtendToUtf16(redirectUrl));
                    }

                    CleanupJsonParser(jsonRootObject);
                }
                else
                {
                    ULONG bufferLength = 0;

                    if (!WinHttpQueryOption(requestHandle, WINHTTP_OPTION_URL, NULL, &bufferLength))
                    {
                        PPH_STRING buffer = PhCreateStringEx(NULL, bufferLength);

                        if (WinHttpQueryOption(requestHandle, WINHTTP_OPTION_URL, buffer->Buffer, &bufferLength))
                        {
                            PhMoveReference(&context->LaunchCommand, PhDuplicateString(buffer));
                        }

                        PhDereferenceObject(buffer);
                    }
                }
            }
            break;
        case MENUITEM_JOTTI_UPLOAD:
            {
                PSTR buffer = NULL;
                PSTR redirectUrl;
                ULONG bufferLength;
                PVOID rootJsonObject;

                if (!ReadRequestString(requestHandle, &buffer, &bufferLength))
                {
                    RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                    goto CleanupExit;
                }

                if (rootJsonObject = CreateJsonParser(buffer))
                {
                    if (redirectUrl = GetJsonValueAsString(rootJsonObject, "redirecturl"))
                    {
                        PhMoveReference(&context->LaunchCommand, PhFormatString(L"http://virusscan.jotti.org%hs", redirectUrl));
                    }

                    CleanupJsonParser(rootJsonObject);
                }
            }
            break;
        }
    }
    else
    {
        RaiseUploadError(context, L"Unable to complete the request", STATUS_FVE_PARTIAL_METADATA);
        goto CleanupExit;
    }

    if (!context->UploadThreadHandle)
        goto CleanupExit;

    if (!PhIsNullOrEmptyString(context->LaunchCommand))
    {
        PostMessage(context->DialogHandle, UM_LAUNCH, 0, 0);
    }
    else
    {
        RaiseUploadError(context, L"Unable to complete the Launch request (please try again after a few minutes)", ERROR_INVALID_DATA);
    }

CleanupExit:

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
    BOOLEAN success = FALSE;
    LARGE_INTEGER fileSize64;
    PPH_BYTES subRequestBuffer = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;
    PSERVICE_INFO serviceInfo = NULL;
    PPH_STRING hashString = NULL;
    PPH_STRING subObjectName = NULL;
    HANDLE fileHandle = NULL;
    PPH_STRING phVersion = NULL;
    PPH_STRING userAgent = NULL;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    //context->Extension = VirusTotalGetCachedResult(context->FileName);
    serviceInfo = GetUploadServiceInfo(context->Service);

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        context->FileName->Buffer,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        RaiseUploadError(context, L"Unable to open the file", RtlNtStatusToDosError(status));
        goto CleanupExit;
    }

    if (NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize64)))
    {
        if (context->Service == MENUITEM_VIRUSTOTAL_UPLOAD)
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
        else
        {
            if (fileSize64.QuadPart > 20 * 1024 * 1024) // 20 MB
            {
                RaiseUploadError(context, L"The file is too large (over 20 MB)", ERROR_FILE_TOO_LARGE);
                goto CleanupExit;
            }
        }

        context->FileSize = PhFormatSize(fileSize64.QuadPart, -1);
        context->TotalFileLength = fileSize64.LowPart;
    }

    // Create a user agent string.
    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"ProcessHacker_", phVersion->Buffer);

    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

    if (!(context->HttpHandle = WinHttpOpen(
        userAgent->Buffer,
        proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy,
        proxyConfig.lpszProxyBypass,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        ULONG gzipFlags = WINHTTP_DECOMPRESSION_FLAG_ALL;

        WinHttpSetOption(context->HttpHandle, WINHTTP_OPTION_DECOMPRESSION, &gzipFlags, sizeof(ULONG));
    }

    switch (context->Service)
    {
    case MENUITEM_VIRUSTOTAL_UPLOAD:
        {
            PSTR uploadUrl = NULL;
            PSTR quote = NULL;
            PVOID rootJsonObject;

            if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, Sha256HashAlgorithm, &hashString)))
            {
                RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                goto CleanupExit;
            }

            subObjectName = PhConcatStrings2(L"/file/upload/?sha256=", hashString->Buffer);

            if (PhIsNullOrEmptyString(context->KeyString))
            {
                // Create the default launch URL
                PhMoveReference(&context->LaunchCommand, PhFormatString(
                    L"https://www.virustotal.com/file/%s/analysis/",
                    PhGetString(hashString)
                    ));
            }

            if (!(subRequestBuffer = PerformSubRequest(
                context,
                serviceInfo->HostName,
                serviceInfo->HostPort,
                serviceInfo->HostFlags,
                subObjectName->Buffer
                )))
            {
                goto CleanupExit;
            }

            if (rootJsonObject = CreateJsonParser(subRequestBuffer->Buffer))
            {  
                if (context->FileExists = GetJsonValueAsBool(rootJsonObject, "file_exists"))
                {
                    INT64 detected = 0;
                    INT64 detectedMax = 0;
                    PVOID detectionRatio;

                    if (detectionRatio = JsonGetObject(rootJsonObject, "detection_ratio"))
                    {
                        detected = GetJsonArrayUlong(detectionRatio, 0);
                        detectedMax = GetJsonArrayUlong(detectionRatio, 1);
                    }

                    context->Detected = PhFormatString(L"%I64d", detected);
                    context->MaxDetected = PhFormatString(L"%I64d", detectedMax);
                    context->UploadUrl = PhZeroExtendToUtf16(GetJsonValueAsString(rootJsonObject, "upload_url"));
                    context->ReAnalyseUrl = PhZeroExtendToUtf16(GetJsonValueAsString(rootJsonObject, "reanalyse_url"));
                    context->LastAnalysisUrl = PhZeroExtendToUtf16(GetJsonValueAsString(rootJsonObject, "last_analysis_url"));
                    context->FirstAnalysisDate = PhZeroExtendToUtf16(GetJsonValueAsString(rootJsonObject, "first_analysis_date"));
                    context->LastAnalysisDate = PhZeroExtendToUtf16(GetJsonValueAsString(rootJsonObject, "last_analysis_date"));
                    context->LastAnalysisAgo = PhZeroExtendToUtf16(GetJsonValueAsString(rootJsonObject, "last_analysis_ago"));

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

                    if (context->VtApiUpload && !PhIsNullOrEmptyString(context->KeyString))
                    {
                        PhMoveReference(&context->UploadUrl, PhFormatString(
                            L"%s%s?apikey=%s&resource=%s",
                            L"https://www.virustotal.com",
                            L"/vtapi/v2/file/scan",
                            PhGetString(context->KeyString),
                            PhGetString(hashString)
                            ));
                    }

                    if (!PhIsNullOrEmptyString(context->UploadUrl))
                    {
                        success = TRUE;
                        PostMessage(context->DialogHandle, UM_EXISTS, 0, 0);
                    }
                }
                else
                {
                    context->UploadUrl = PhZeroExtendToUtf16(GetJsonValueAsString(rootJsonObject, "upload_url"));

                    // No file found... Start the upload.
                    if (!PhIsNullOrEmptyString(context->UploadUrl))
                    {
                        success = TRUE;
                        PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
                    }
                }
 
                CleanupJsonParser(rootJsonObject);
            }
        }
        break;
    case MENUITEM_JOTTI_UPLOAD:
        {
            // Create the default upload URL
            context->UploadUrl = PhFormatString(L"https://virusscan.jotti.org%s", serviceInfo->UploadObjectName);

            // No file found... Start the upload.
            if (!PhIsNullOrEmptyString(context->UploadUrl))
            {
                success = TRUE;
                PostMessage(context->DialogHandle, UM_UPLOAD, 0, 0);
            }
        }
        break;
    }

CleanupExit:

    if (context->DialogHandle && !success)
    {
        PostMessage(context->DialogHandle, UM_ERROR, 0, 0);
    }

    PhClearReference(&phVersion);
    PhClearReference(&userAgent);
    PhClearReference(&hashString);
    PhClearReference(&subObjectName);
    PhClearReference(&subRequestBuffer);  

    if (requestHandle)
        WinHttpCloseHandle(requestHandle);

    if (connectHandle)
        WinHttpCloseHandle(connectHandle);

    if (fileHandle)
        NtClose(fileHandle);

    PhDereferenceObject(context);

    return status;
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            TaskDialogFreeContext(context);

            RemoveWindowSubclass(hwndDlg, TaskDialogSubclassProc, uIdSubclass);
        }
        break;
    case UM_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case UM_UPLOAD:
        {
            ShowVirusTotalProgressDialog(context);
        }
        break;
    case UM_EXISTS:
        {
            ShowFileFoundDialog(context);
        }
        break;
    case UM_LAUNCH:
        {
            if (!PhIsNullOrEmptyString(context->LaunchCommand))
            {
                PhShellExecute(hwndDlg, context->LaunchCommand->Buffer, NULL);
            }

            PostQuitMessage(0);
        }
        break;
    case UM_ERROR:
        {
            VirusTotalShowErrorDialog(context);
        }
        break;
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
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
            TaskDialogCreateIcons(context);

            if (SUCCEEDED(CoCreateInstance(&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskbarList3, &context->TaskbarListClass)))
            {
                if (!SUCCEEDED(ITaskbarList3_HrInit(context->TaskbarListClass)))
                {
                    ITaskbarList3_Release(context->TaskbarListClass);
                    context->TaskbarListClass = NULL;
                }
            }

            // Subclass the Taskdialog
            SetWindowSubclass(hwndDlg, TaskDialogSubclassProc, 0, (ULONG_PTR)context);

            ShowVirusTotalUploadDialog(context);
        }
        break;
    }

    return S_OK;
}

NTSTATUS ShowUpdateDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    BOOL checked = FALSE;

    PhInitializeAutoPool(&autoPool);

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
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    HANDLE dialogThread;
    PUPLOAD_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        UploadContextType = PhCreateObjectType(L"OnlineChecksObjectType", 0, UploadContextDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    context = (PUPLOAD_CONTEXT)PhCreateObject(sizeof(UPLOAD_CONTEXT), UploadContextType);
    memset(context, 0, sizeof(UPLOAD_CONTEXT));

    context->Service = Service;
    context->FileName = PhDuplicateString(FileName);
    context->BaseFileName = PhGetBaseName(context->FileName);
    context->KeyString = PhCreateString(VIRUSTOTAL_APIKEY);

    if (dialogThread = PhCreateThread(0, ShowUpdateDialogThread, (PVOID)context))
    {
        PostMessage(dialogThread, UM_SHOWDIALOG, 0, 0);
        NtClose(dialogThread);
    }
}
