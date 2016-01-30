  /*
 * Process Hacker Online Checks -
 *   Uploader Window
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2016 dmex
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
#include "json-c/json.h"

static SERVICE_INFO UploadServiceInfo[] =
{
    { UPLOAD_SERVICE_VIRUSTOTAL, L"www.virustotal.com", INTERNET_DEFAULT_HTTPS_PORT, WINHTTP_FLAG_SECURE, L"???", L"file" },
    { UPLOAD_SERVICE_JOTTI, L"virusscan.jotti.org", INTERNET_DEFAULT_HTTPS_PORT, WINHTTP_FLAG_SECURE, L"/en-US/submit-file?isAjax=true", L"sample-file[]" },
    { UPLOAD_SERVICE_CIMA, L"camas.comodo.com", INTERNET_DEFAULT_HTTP_PORT, 0, L"/cgi-bin/submit", L"file" }
};

static json_object_ptr json_get_object(json_object_ptr rootObj, const char* key)
{
    json_object_ptr returnObj;

    if (json_object_object_get_ex(rootObj, key, &returnObj))
    {
        return returnObj;
    }

    return NULL;
}

static HFONT InitializeFont(
    _In_ HWND hwnd
    )
{
    LOGFONT logFont;

    // Create the font handle
    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
    {
        HDC hdc;

        if (hdc = GetDC(hwnd))
        {
            HFONT fontHandle = CreateFont(
                -MulDiv(-14, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                0,
                0,
                0,
                FW_MEDIUM,
                FALSE,
                FALSE,
                FALSE,
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY | ANTIALIASED_QUALITY,
                DEFAULT_PITCH,
                logFont.lfFaceName
                );

            SendMessage(hwnd, WM_SETFONT, (WPARAM)fontHandle, TRUE);

            ReleaseDC(hwnd, hdc);

            return fontHandle;
        }
    }

    return NULL;
}

static BOOL ReadRequestString(
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

static VOID RaiseUploadError(
    _In_ PUPLOAD_CONTEXT Context,
    _In_ PWSTR Error,
    _In_ ULONG ErrorCode
    )
{
    if (Context->DialogHandle)
    {
        PostMessage(
            Context->DialogHandle,
            UM_ERROR,
            0,
            (LPARAM)PhFormatString(L"Error: [%lu] %s", ErrorCode, Error)
            );
    }
}

static PSERVICE_INFO GetUploadServiceInfo(
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

static BOOLEAN PerformSubRequest(
    _In_ PUPLOAD_CONTEXT Context,
    _In_ PWSTR HostName,
    _In_ INTERNET_PORT HostPort,
    _In_ ULONG HostFlags,
    _In_ PWSTR ObjectName,
    _Out_ _Deref_post_z_cap_(*DataLength) PSTR *Data,
    _Out_opt_ PULONG DataLength
    )
{
    BOOLEAN result = FALSE;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;

    __try
    {
        // Connect to the online service.
        if (!(connectHandle = WinHttpConnect(
            Context->HttpHandle,
            HostName,
            HostPort,
            0
            )))
        {
            RaiseUploadError(Context, L"Unable to connect to the service", GetLastError());
            __leave;
        }

        // Create the request.
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
            __leave;
        }

        // Send the request.
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
            __leave;
        }

        // Wait for the send request to complete and recieve the response.
        if (WinHttpReceiveResponse(requestHandle, NULL))
        {
            BYTE buffer[PAGE_SIZE];
            PSTR data;
            ULONG allocatedLength;
            ULONG dataLength;
            ULONG returnLength;

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

            // Ensure that the buffer is null-terminated.
            data[dataLength] = 0;

            *Data = data;

            if (DataLength)
            {
                *DataLength = dataLength;
            }
        }
        else
        {
            RaiseUploadError(Context, L"Unable to receive the response", GetLastError());
            __leave;
        }

        result = TRUE;
    }
    __finally
    {
        if (requestHandle)
            WinHttpCloseHandle(requestHandle);
        if (connectHandle)
            WinHttpCloseHandle(connectHandle);
    }

    return result;
}

static NTSTATUS HashFileAndResetPosition(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER FileSize,
    _In_ ULONG Algorithm,
    _Out_ PVOID Hash
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PH_HASH_CONTEXT hashContext;
    sha256_context sha256;
    ULONG64 bytesRemaining;
    FILE_POSITION_INFORMATION positionInfo;
    UCHAR buffer[PAGE_SIZE];

    bytesRemaining = FileSize->QuadPart;

    switch (Algorithm)
    {
    case HASH_SHA1:
        PhInitializeHash(&hashContext, Sha1HashAlgorithm);
        break;
    case HASH_SHA256:
        sha256_starts(&sha256);
        break;
    }

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

        switch (Algorithm)
        {
        case HASH_SHA1:
            PhUpdateHash(&hashContext, buffer, (ULONG)iosb.Information);
            break;
        case HASH_SHA256:
            sha256_update(&sha256, (PUCHAR)buffer, (ULONG)iosb.Information);
            break;
        }

        bytesRemaining -= (ULONG)iosb.Information;
    }

    if (status == STATUS_END_OF_FILE)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        switch (Algorithm)
        {
        case HASH_SHA1:
            PhFinalHash(&hashContext, Hash, 20, NULL);
            break;
        case HASH_SHA256:
            sha256_finish(&sha256, Hash);
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

    return status;
}

static NTSTATUS UploadFileThreadStart(
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

    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK isb;
    PSERVICE_INFO serviceInfo = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;

    PPH_STRING postBoundary = NULL;
    PPH_BYTES asciiPostData = NULL;
    PPH_BYTES asciiFooterData = NULL;
    PH_STRING_BUILDER httpRequestHeaders = { 0 };
    PH_STRING_BUILDER httpPostHeader = { 0 };
    PH_STRING_BUILDER httpPostFooter = { 0 };
    BYTE buffer[PAGE_SIZE];

    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    serviceInfo = GetUploadServiceInfo(context->Service);

    __try
    {
        // Open the file and check its size.
        status = PhCreateFileWin32(
            &fileHandle,
            context->FileName->Buffer,
            FILE_GENERIC_READ,
            0,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (!NT_SUCCESS(status))
        {
            RaiseUploadError(context, L"Unable to open the file", RtlNtStatusToDosError(status));
            __leave;
        }

        // Connect to the online service.
        if (!(connectHandle = WinHttpConnect(
            context->HttpHandle,
            serviceInfo->HostName,
            serviceInfo->HostPort,
            0
            )))
        {
            RaiseUploadError(context, L"Unable to connect to the service", GetLastError());
            __leave;
        }

        // Create the request.
        if (!(requestHandle = WinHttpOpenRequest(
            connectHandle,
            L"POST",
            context->ObjectName->Buffer,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_REFRESH | serviceInfo->HostFlags
            )))
        {
            RaiseUploadError(context, L"Unable to create the request", GetLastError());
            __leave;
        }

        if (context->Service == UPLOAD_SERVICE_JOTTI)
        {
            PPH_STRING ajaxHeader;

            ajaxHeader = PhCreateString(L"X-Requested-With: XMLHttpRequest");

            WinHttpAddRequestHeaders(
                requestHandle,
                ajaxHeader->Buffer,
                (ULONG)ajaxHeader->Length / sizeof(WCHAR),
                WINHTTP_ADDREQ_FLAG_ADD
                );

            PhDereferenceObject(ajaxHeader);
        }

        // Create and POST data.
        PhInitializeStringBuilder(&httpRequestHeaders, MAX_PATH);
        PhInitializeStringBuilder(&httpPostHeader, MAX_PATH);
        PhInitializeStringBuilder(&httpPostFooter, MAX_PATH);

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

        if (context->Service == UPLOAD_SERVICE_JOTTI)
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
                context->BaseFileName->Buffer
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
                context->BaseFileName->Buffer
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
            __leave;
        }

        // All until now has been just for this; Calculate the total request length.
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
            __leave;
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
            __leave;
        }

        // Upload the file...
        while (TRUE)
        {
            status = NtReadFile(
                fileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                PAGE_SIZE,
                NULL,
                NULL
                );

            if (!NT_SUCCESS(status))
                break;

            if (!WinHttpWriteData(requestHandle, buffer, (ULONG)isb.Information, &totalWriteLength))
            {
                RaiseUploadError(context, L"Unable to upload the file data", GetLastError());
                __leave;
            }

            totalUploadedLength += totalWriteLength;

            // Query the current time
            PhQuerySystemTime(&timeNow);

            // Calculate the number of ticks
            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
            timeBitsPerSecond = totalUploadedLength / __max(timeTicks, 1);

            {
                FLOAT percent = ((FLOAT)totalUploadedLength / context->TotalFileLength * 100);
                PPH_STRING totalLength = PhFormatSize(context->TotalFileLength, -1);
                PPH_STRING totalUploaded = PhFormatSize(totalUploadedLength, -1);
                PPH_STRING totalSpeed = PhFormatSize(timeBitsPerSecond, -1);

                PPH_STRING statusMessage = PhFormatString(
                    L"%s of %s @ %s/s",
                    totalUploaded->Buffer,
                    totalLength->Buffer,
                    totalSpeed->Buffer
                    );

                Static_SetText(context->StatusHandle, statusMessage->Buffer);

                // Update the progress bar position
                PostMessage(context->ProgressHandle, PBM_SETPOS, (INT)percent, 0);

                PhDereferenceObject(statusMessage);
                PhDereferenceObject(totalSpeed);
                PhDereferenceObject(totalLength);
                PhDereferenceObject(totalUploaded);
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
            __leave;
        }

        // Wait for the send request to complete and recieve the response.
        if (!WinHttpReceiveResponse(requestHandle, NULL))
        {
            RaiseUploadError(context, L"Unable to receive the response", GetLastError());
            __leave;
        }

        // Handle service-specific actions.
        WinHttpQueryHeaders(
            requestHandle,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL,
            &httpStatus,
            &httpStatusLength,
            NULL
            );

        if (httpStatus == HTTP_STATUS_OK || httpStatus == HTTP_STATUS_REDIRECT_METHOD || httpStatus == HTTP_STATUS_REDIRECT)
        {
            switch (context->Service)
            {
            case UPLOAD_SERVICE_VIRUSTOTAL:
                {
                    ULONG bufferLength = 0;

                    // Use WinHttpQueryOption to obtain a buffer size.
                    if (!WinHttpQueryOption(requestHandle, WINHTTP_OPTION_URL, NULL, &bufferLength))
                    {
                        PPH_STRING buffer = PhCreateStringEx(NULL, bufferLength);

                        // Use WinHttpQueryOption again, this time to retrieve the URL in the new buffer
                        if (WinHttpQueryOption(requestHandle, WINHTTP_OPTION_URL, buffer->Buffer, &bufferLength))
                        {
                            // Format the retrieved URL...
                            context->LaunchCommand = PhDuplicateString(buffer);
                        }

                        PhDereferenceObject(buffer);
                    }
                }
                break;
            case UPLOAD_SERVICE_JOTTI:
                {
                    PSTR hrefEquals = NULL;
                    PSTR quote = NULL;
                    PSTR buffer = NULL;
                    ULONG bufferLength = 0;
                    json_object_ptr rootJsonObject;

                    //This service returns some json that redirects the user to the new location.
                    if (!ReadRequestString(requestHandle, &buffer, &bufferLength))
                    {
                        RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                        __leave;
                    }

                    if (rootJsonObject = json_tokener_parse(buffer))
                    {
                        PSTR redirectUrl = json_object_get_string(json_get_object(rootJsonObject, "redirecturl"));

                        context->LaunchCommand = PhFormatString(
                            L"http://virusscan.jotti.org%hs",
                            redirectUrl
                            );

                        json_object_put(rootJsonObject);
                    }
                }
                break;
            case UPLOAD_SERVICE_CIMA:
                {
                    PSTR urlEquals = NULL;
                    PSTR quote = NULL;
                    PSTR buffer = NULL;
                    ULONG bufferLength = 0;

                    // This service returns some HTML that redirects the user to the new location.
                    if (!ReadRequestString(requestHandle, &buffer, &bufferLength))
                    {
                        RaiseUploadError(context, L"Unable to complete the CIMA request", GetLastError());
                        __leave;
                    }

                    // The HTML looks like this:
                    // <META http-equiv="Refresh" content="0; url=...">
                    urlEquals = strstr(buffer, "url=");

                    if (urlEquals)
                    {
                        urlEquals += 4;
                        quote = strchr(urlEquals, '"');

                        if (quote)
                        {
                            context->LaunchCommand = PhFormatString(
                                L"http://camas.comodo.com%.*S",
                                quote - urlEquals,
                                urlEquals
                                );
                        }
                    }
                }
                break;
            }
        }
        else
        {
            RaiseUploadError(context, L"Unable to complete the request", STATUS_FVE_PARTIAL_METADATA);
            __leave;
        }

        if (!PhIsNullOrEmptyString(context->LaunchCommand))
        {
            PostMessage(context->DialogHandle, UM_LAUNCH, 0, 0);
        }
        else
        {
            RaiseUploadError(context, L"Unable to complete the Launch request (please try again after a few minutes)", ERROR_INVALID_DATA);
            __leave;
        }
    }
    __finally
    {
        if (postBoundary)
        {
            PhDereferenceObject(postBoundary);
        }

        if (asciiFooterData)
        {
            PhDereferenceObject(asciiFooterData);
        }

        if (asciiPostData)
        {
            PhDereferenceObject(asciiPostData);
        }

        if (httpPostFooter.String)
        {
            PhDeleteStringBuilder(&httpPostFooter);
        }

        if (httpPostHeader.String)
        {
            PhDeleteStringBuilder(&httpPostHeader);
        }

        if (httpRequestHeaders.String)
        {
            PhDeleteStringBuilder(&httpRequestHeaders);
        }

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            NtClose(fileHandle);
        }
    }

    return status;
}

static NTSTATUS UploadCheckThreadStart(
    _In_ PVOID Parameter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN fileExists = FALSE;
    LARGE_INTEGER fileSize64;
    PSTR subRequestBuffer = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;
    PSERVICE_INFO serviceInfo = NULL;
    PPH_STRING hashString = NULL;
    PPH_STRING subObjectName = NULL;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;

    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    serviceInfo = GetUploadServiceInfo(context->Service);

    __try
    {
        // Open the file and check its size.
        status = PhCreateFileWin32(
            &fileHandle,
            context->FileName->Buffer,
            FILE_GENERIC_READ,
            0,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (!NT_SUCCESS(status))
        {
            RaiseUploadError(context, L"Unable to open the file", RtlNtStatusToDosError(status));
            __leave;
        }

        if (NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize64)))
        {
            if (context->Service == UPLOAD_SERVICE_VIRUSTOTAL)
            {
                if (fileSize64.QuadPart > 128 * 1024 * 1024) // 128 MB
                {
                    RaiseUploadError(context, L"The file is too large (over 128 MB)", ERROR_FILE_TOO_LARGE);
                    __leave;
                }
            }
            else
            {
                if (fileSize64.QuadPart > 20 * 1024 * 1024) // 20 MB
                {
                    RaiseUploadError(context, L"The file is too large (over 20 MB)", ERROR_FILE_TOO_LARGE);
                    __leave;
                }
            }

            context->TotalFileLength = fileSize64.LowPart;
        }

        // Get proxy configuration and create winhttp handle (used for all winhttp sessions + requests).
        {
            PPH_STRING phVersion = NULL;
            PPH_STRING userAgent = NULL;
            WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

            // Create a user agent string.
            phVersion = PhGetPhVersion();
            userAgent = PhConcatStrings2(L"ProcessHacker_", phVersion->Buffer);

            // Query the current system proxy
            WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

            // Open the HTTP session with the system proxy configuration if available
            context->HttpHandle = WinHttpOpen(
                userAgent->Buffer,
                proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                proxyConfig.lpszProxy,
                proxyConfig.lpszProxyBypass,
                0
                );

            PhClearReference(&phVersion);
            PhClearReference(&userAgent);

            if (!context->HttpHandle)
                __leave;

            if (WindowsVersion >= WINDOWS_8_1)
            {
                // Enable GZIP and DEFLATE support on Windows 8.1 and above using undocumented flags.
                ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

                WinHttpSetOption(
                    context->HttpHandle,
                    WINHTTP_OPTION_DECOMPRESSION,
                    &httpFlags,
                    sizeof(ULONG)
                    );
            }
        }

        switch (context->Service)
        {
        case UPLOAD_SERVICE_VIRUSTOTAL:
            {
                PSTR uploadUrl = NULL;
                PSTR quote = NULL;
                ULONG bufferLength = 0;
                UCHAR hash[32];
                json_object_ptr rootJsonObject;

                if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA256, hash)))
                {
                    RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                    __leave;
                }

                hashString = PhBufferToHexString(hash, 32);
                subObjectName = PhConcatStrings2(L"/file/upload/?sha256=", hashString->Buffer);
                context->LaunchCommand = PhFormatString(L"http://www.virustotal.com/file/%s/analysis/", hashString->Buffer);

                if (!PerformSubRequest(
                    context,
                    serviceInfo->HostName,
                    serviceInfo->HostPort,
                    serviceInfo->HostFlags,
                    subObjectName->Buffer,
                    &subRequestBuffer,
                    &bufferLength
                    ))
                {
                    __leave;
                }

                if (rootJsonObject = json_tokener_parse(subRequestBuffer))
                {
                    json_bool file_Exists;
                    PSTR uploadUrl;

                    file_Exists  = json_object_get_boolean(json_get_object(rootJsonObject, "file_exists"));
                    uploadUrl = json_object_get_string(json_get_object(rootJsonObject, "upload_url"));

                    if (file_Exists)
                    {
                        fileExists = TRUE;
                    }

                    context->ObjectName = PhZeroExtendToUtf16(uploadUrl + strlen("https://www.virustotal.com"));

                    json_object_put(rootJsonObject);
                }

                // Create the default upload URL
                if (!context->ObjectName)
                    context->ObjectName = PhCreateString(serviceInfo->UploadObjectName);
            }
            break;
        case UPLOAD_SERVICE_JOTTI:
            {
                // Create the default upload URL
                if (!context->ObjectName)
                    context->ObjectName = PhCreateString(serviceInfo->UploadObjectName);
            }
            break;
        case UPLOAD_SERVICE_CIMA:
            {
                PSTR quote = NULL;
                ULONG bufferLength = 0;
                UCHAR hash[32];
                ULONG status = 0;
                ULONG statusLength = sizeof(statusLength);

                if (!NT_SUCCESS(status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA256, hash)))
                {
                    RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                    __leave;
                }

                hashString = PhBufferToHexString(hash, 32);
                subObjectName = PhConcatStrings2(L"/cgi-bin/submit?file=", hashString->Buffer);
                context->LaunchCommand = PhFormatString(L"http://camas.comodo.com/cgi-bin/submit?file=%s", hashString->Buffer);

                // Connect to the CIMA online service.
                if (!(connectHandle = WinHttpConnect(
                    context->HttpHandle,
                    serviceInfo->HostName,
                    INTERNET_DEFAULT_HTTP_PORT,
                    0
                    )))
                {
                    RaiseUploadError(context, L"Unable to connect to the CIMA service", GetLastError());
                    __leave;
                }

                // Create the request.
                if (!(requestHandle = WinHttpOpenRequest(
                    connectHandle,
                    NULL,
                    subObjectName->Buffer,
                    NULL,
                    WINHTTP_NO_REFERER,
                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                    WINHTTP_FLAG_REFRESH
                    )))
                {
                    RaiseUploadError(context, L"Unable to create the CIMA request", GetLastError());
                    __leave;
                }

                // Send the request.
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
                    RaiseUploadError(context, L"Unable to send the CIMA request", GetLastError());
                    __leave;
                }

                // Wait for the send request to complete and recieve the response.
                if (!WinHttpReceiveResponse(requestHandle, NULL))
                {
                    RaiseUploadError(context, L"Unable to recieve the CIMA response", GetLastError());
                    __leave;
                }

                WinHttpQueryHeaders(
                    requestHandle,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    NULL,
                    &status,
                    &statusLength,
                    NULL
                    );

                if (status == HTTP_STATUS_OK)
                {
                    fileExists = TRUE;
                }

                if (!context->ObjectName)
                {
                    // Create the default upload URL
                    context->ObjectName = PhCreateString(serviceInfo->UploadObjectName);
                }
            }
            break;
        }

        // Do we need to prompt the user?
        if (fileExists && !PhIsNullOrEmptyString(context->LaunchCommand))
        {
            PostMessage(context->DialogHandle, UM_EXISTS, 0, 0);
            __leave;
        }

        // No existing file found... Start the upload.
        if (!NT_SUCCESS(UploadFileThreadStart(context)))
            __leave;
    }
    __finally
    {
        PhClearReference(&hashString);
        PhClearReference(&subObjectName);

        if (requestHandle)
        {
            WinHttpCloseHandle(requestHandle);
        }

        if (connectHandle)
        {
            WinHttpCloseHandle(connectHandle);
        }

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            NtClose(fileHandle);
        }
    }

    return status;
}

static INT_PTR CALLBACK UploadDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PUPLOAD_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PUPLOAD_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PUPLOAD_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            PhClearReference(&context->FileName);
            PhClearReference(&context->BaseFileName);
            PhClearReference(&context->WindowFileName);
            PhClearReference(&context->LaunchCommand);
            PhClearReference(&context->ObjectName);

            if (context->MessageFont)
                DeleteObject(context->MessageFont);

            if (context->HttpHandle)
                WinHttpCloseHandle(context->HttpHandle);

            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE dialogThread = NULL;
            HWND parentWindow = GetParent(hwndDlg);

            PhCenterWindow(hwndDlg, (IsWindowVisible(parentWindow) && !IsIconic(parentWindow)) ? parentWindow : NULL);

            context->DialogHandle = hwndDlg;
            context->StatusHandle = GetDlgItem(hwndDlg, IDC_STATUS);
            context->ProgressHandle = GetDlgItem(hwndDlg, IDC_PROGRESS1);
            context->MessageHandle = GetDlgItem(hwndDlg, IDC_MESSAGE);
            context->MessageFont = InitializeFont(context->MessageHandle);
            context->WindowFileName = PhFormatString(L"Uploading: %s", context->BaseFileName->Buffer);
            context->UploadServiceState = PhUploadServiceChecking;

            // Reset the window status...
            Static_SetText(context->MessageHandle, context->WindowFileName->Buffer);

            switch (context->Service)
            {
            case UPLOAD_SERVICE_VIRUSTOTAL:
                Static_SetText(hwndDlg, L"Uploading to VirusTotal...");
                break;
            case UPLOAD_SERVICE_JOTTI:
                Static_SetText(hwndDlg, L"Uploading to Jotti...");
                break;
            case UPLOAD_SERVICE_CIMA:
                Static_SetText(hwndDlg, L"Uploading to Comodo...");
                break;
            }

            if (dialogThread = PhCreateThread(0, UploadCheckThreadStart, (PVOID)context))
                NtClose(dialogThread);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDYES:
                {
                    context->UploadServiceState = PhUploadServiceMaximum;

                    if (!PhIsNullOrEmptyString(context->LaunchCommand))
                    {
                        PhShellExecute(hwndDlg, context->LaunchCommand->Buffer, NULL);
                    }

                    PostQuitMessage(0);
                }
                break;
            case IDNO:
                {
                    if (context->UploadServiceState == PhUploadServiceViewReport)
                    {
                        HANDLE dialogThread = NULL;

                        // Set state to uploading...
                        context->UploadServiceState = PhUploadServiceUploading;

                        // Reset the window status...
                        Static_SetText(context->MessageHandle, context->WindowFileName->Buffer);
                        Static_SetText(GetDlgItem(hwndDlg, IDNO), L"Cancel");
                        Control_Visible(GetDlgItem(hwndDlg, IDYES), FALSE);

                        // Start the upload thread...
                        if (dialogThread = PhCreateThread(0, UploadFileThreadStart, (PVOID)context))
                            NtClose(dialogThread);
                    }
                    else
                    {
                        context->UploadServiceState = PhUploadServiceMaximum;
                        PostQuitMessage(0);
                    }
                }
                break;
            case IDCANCEL:
                {
                    context->UploadServiceState = PhUploadServiceMaximum;
                    PostQuitMessage(0);
                }
            }
            break;
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;

            // Check for our static label and change the color.
            if (GetDlgCtrlID(hwndChild) == IDC_MESSAGE)
            {
                SetTextColor(hDC, RGB(19, 112, 171));
            }

            // Set a transparent background for the control backcolor.
            SetBkMode(hDC, TRANSPARENT);

            // set window background color.
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    case UM_EXISTS:
        {
            context->UploadServiceState = PhUploadServiceViewReport;

            Static_SetText(GetDlgItem(hwndDlg, IDNO), L"No");
            Control_Visible(GetDlgItem(hwndDlg, IDYES), TRUE);

            Static_SetText(context->MessageHandle, L"File already analysed.");
            Static_SetText(context->StatusHandle, L"View existing report?");
        }
        break;
    case UM_LAUNCH:
        {
            context->UploadServiceState = PhUploadServiceMaximum;

            if (!PhIsNullOrEmptyString(context->LaunchCommand))
            {
                PhShellExecute(hwndDlg, context->LaunchCommand->Buffer, NULL);
            }

            PostQuitMessage(0);
        }
        break;
    case UM_ERROR:
        {
            PPH_STRING errorMessage = (PPH_STRING)lParam;

            context->UploadServiceState = PhUploadServiceMaximum;

            if (errorMessage)
            {
                Static_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), errorMessage->Buffer);
                PhClearReference(&errorMessage);
            }
            else
            {
                Static_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), L"Error");
            }

            Static_SetText(GetDlgItem(hwndDlg, IDC_STATUS), L"");
            Static_SetText(GetDlgItem(hwndDlg, IDNO), L"Close");
        }
        break;
    }

    return FALSE;
}

static NTSTATUS PhUploadToDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND dialogHandle;
    PH_AUTO_POOL autoPool;
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    PhInitializeAutoPool(&autoPool);

    dialogHandle = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROGRESS),
        PhMainWndHandle,
        UploadDlgProc,
        (LPARAM)Parameter
        );

    ShowWindow(dialogHandle, SW_SHOW);
    SetForegroundWindow(dialogHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(dialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    DestroyWindow(dialogHandle);

    return STATUS_SUCCESS;
}

VOID UploadToOnlineService(
    _In_ PPH_STRING FileName,
    _In_ ULONG Service
    )
{
    HANDLE dialogThread = NULL;
    PUPLOAD_CONTEXT context;

    context = (PUPLOAD_CONTEXT)PhAllocate(sizeof(UPLOAD_CONTEXT));
    memset(context, 0, sizeof(UPLOAD_CONTEXT));

    context->Service = Service;
    context->FileName = PhDuplicateString(FileName);
    context->BaseFileName = PhGetBaseName(context->FileName);

    if (dialogThread = PhCreateThread(0, PhUploadToDialogThreadStart, (PVOID)context))
        NtClose(dialogThread);
}