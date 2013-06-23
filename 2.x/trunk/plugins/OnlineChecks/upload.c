/*
 * Process Hacker Online Checks -
 *   uploader
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2013 dmex
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

static SERVICE_INFO UploadServiceInfo[] =
{
    { UPLOAD_SERVICE_VIRUSTOTAL, L"www.virustotal.com", INTERNET_DEFAULT_HTTPS_PORT, WINHTTP_FLAG_SECURE, L"???", L"file" },
    { UPLOAD_SERVICE_JOTTI, L"virusscan.jotti.org", INTERNET_DEFAULT_HTTP_PORT, 0, L"/processupload.php", L"scanfile" },
    { UPLOAD_SERVICE_CIMA, L"camas.comodo.com", INTERNET_DEFAULT_HTTP_PORT, 0, L"/cgi-bin/submit", L"file" }
};

static NTSTATUS PhUploadToDialogThreadStart(
    __in PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;    
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;

    PhInitializeAutoPool(&autoPool);

    context->DialogHandle = CreateDialogParam(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROGRESS),
        PhMainWndHandle,
        UploadDlgProc,
        (LPARAM)Parameter
        );
            
    ShowWindow(context->DialogHandle, SW_SHOW);
    SetForegroundWindow(context->DialogHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(context->DialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    DestroyWindow(context->DialogHandle);
    PhFree(context);
    return STATUS_SUCCESS;
}

static HFONT InitializeFont(
    __in HWND hwndDlg
    )
{
    LOGFONT logFont = { 0 };
    HFONT fontHandle = NULL;

    logFont.lfHeight = 20;
    logFont.lfWeight = FW_MEDIUM;
    logFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;

    wcscpy_s(logFont.lfFaceName, _countof(logFont.lfFaceName),
        WindowsVersion > WINDOWS_XP ? L"Segoe UI" : L"MS Shell Dlg 2"
        );

    fontHandle = CreateFontIndirect(&logFont);

    if (fontHandle)
    {
        SendMessage(hwndDlg, WM_SETFONT, (WPARAM)fontHandle, FALSE);
        return fontHandle;
    }

    return NULL;
}

static BOOL ReadRequestString(
    __in HINTERNET Handle,
    __out_z PSTR* Data,
    __out ULONG* DataLength
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
    RtlZeroMemory(buffer, PAGE_SIZE);

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
    __in PUPLOAD_CONTEXT Context,
    __in PWSTR Error,
    __in_opt ULONG ErrorCode
    )
{
    PPH_STRING errorMessage = NULL;

    if (ErrorCode)
    {
        errorMessage = PhGetWin32Message(ErrorCode);

        if (!errorMessage)
            errorMessage = PhFormatString(L"Error %u", ErrorCode);
    }

    if (errorMessage)
        Context->ErrorMessage = PhConcatStrings(3, Error, L": ", errorMessage->Buffer);
    else
        Context->ErrorMessage = PhConcatStrings2(Error, L".");

    PhSwapReference(&errorMessage, NULL);

    if (Context->DialogHandle)
        PostMessage(Context->DialogHandle, UM_ERROR, 0, 0);
}

static PSERVICE_INFO GetUploadServiceInfo(
    __in ULONG Id
    )
{
    ULONG i;

    for (i = 0; i < _countof(UploadServiceInfo); i++)
    {
        if (UploadServiceInfo[i].Id == Id)
            return &UploadServiceInfo[i];
    }

    return NULL;
}

static BOOLEAN PerformSubRequest(
    __in PUPLOAD_CONTEXT Context,
    __in PWSTR HostName,
    __in PWSTR ObjectName,
    __out_bcount(DataLength) PSTR* Data,
    __out_opt PULONG DataLength
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
            INTERNET_DEFAULT_HTTP_PORT,
            0
            )))
        {
            RaiseUploadError(Context, L"Unable to connect to the service", GetLastError());
            __leave;
        }

        // Create the request.
        if (!(requestHandle = WinHttpOpenRequest(
            connectHandle,
            NULL, // GET
            ObjectName,
            NULL,// HTTP/1.1
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_REFRESH
            )))
        {
            RaiseUploadError(Context, L"Unable to create the request", GetLastError());
            __leave;
        }

        // Send the request.
        if (!WinHttpSendRequest(requestHandle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
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
    __in HANDLE FileHandle,
    __in PLARGE_INTEGER FileSize,
    __in ULONG Algorithm,
    __out PVOID Hash
    )
{
    NTSTATUS status;
    UCHAR buffer[PAGE_SIZE * 4];
    IO_STATUS_BLOCK iosb;
    PH_HASH_CONTEXT hashContext;
    sha256_context sha256;
    ULONG64 bytesRemaining;
    FILE_POSITION_INFORMATION positionInfo;

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
        status = NtSetInformationFile(FileHandle, &iosb, &positionInfo, sizeof(FILE_POSITION_INFORMATION), FilePositionInformation);
    }

    return status;
}

static NTSTATUS UploadWorkerThreadStart(
    __in PUPLOAD_CONTEXT context
    )
{ 
    time_t timeStart = 0;
    time_t timeTransferred = 0;  
    ULONG totalFileLength = 0;
    ULONG totalPostHeaderWritten = 0;
    ULONG totalPostFooterWritten = 0;
    ULONG totalReadLength = 0;
    ULONG totalUploadLength = 0;
    ULONG httpPostSeed = 0;

    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    LARGE_INTEGER fileSize64;
    PSTR subRequestBuffer = NULL;

    HANDLE fileHandle = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;
    PSERVICE_INFO serviceInfo = NULL;
    PH_STRING_BUILDER httpRequestHeaders = { 0 };
    PH_STRING_BUILDER httpPostHeader = { 0 };
    PH_STRING_BUILDER httpPostFooter = { 0 };
    PPH_ANSI_STRING ansiPostData = NULL;
    PPH_ANSI_STRING ansiFooterData = NULL;
    PPH_STRING userAgent = NULL;
    PPH_STRING objectName = NULL;
    PPH_STRING postBoundary = NULL;
    PPH_STRING baseFileName = PhGetBaseName(context->FileName);
    PPH_STRING windowStatus = PhFormatString(
        L"Uploading: %s",
        baseFileName->Buffer
        );

    SetWindowText(context->MessageHandle, windowStatus->Buffer);
    PhDereferenceObject(windowStatus);

    __try
    {
        if (!(serviceInfo = GetUploadServiceInfo(context->Service)))
        {
            RaiseUploadError(context, L"The service type is invalid", 0);
            __leave;
        }

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

        if (NT_SUCCESS(status))
        {
            if (NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize64)))
            {
                if (fileSize64.QuadPart > 20 * 1024 * 1024) // 20 MB
                {
                    RaiseUploadError(context, L"The file is too large (over 20 MB)", 0);
                    __leave;
                }

                totalFileLength = fileSize64.LowPart;
            }
        }

        if (!NT_SUCCESS(status))
        {
            RaiseUploadError(context, L"Unable to open the file", RtlNtStatusToDosError(status));
            __leave;
        }

        // Get proxy configuration and create winhttp handle (used for all winhttp sessions + requests).
        {
            PPH_STRING phVersion = PhGetPhVersion();
            WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxycfg = { 0 };

            // Create a user agent string.
            userAgent = PhConcatStrings2(L"Process Hacker ", phVersion->Buffer);

            if (!WinHttpGetIEProxyConfigForCurrentUser(&proxycfg))
            {
                RaiseUploadError(context, L"WinHttpGetIEProxyConfigForCurrentUser Failure", GetLastError());
                __leave;
            }

            // Create the internet handle.
            if (!(context->HttpHandle = WinHttpOpen(
                userAgent->Buffer,
                proxycfg.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                proxycfg.lpszProxy,
                proxycfg.lpszProxyBypass,
                0
                )))
            {
                RaiseUploadError(context, L"Unable to initialize internet access", GetLastError());
                __leave;
            }

            PhDereferenceObject(phVersion);
            PhDereferenceObject(userAgent);
        }

        switch (context->Service)
        {
        case UPLOAD_SERVICE_VIRUSTOTAL:
            {
                PPH_STRING hashString = NULL;
                PPH_STRING subObjectName = NULL;
                PSTR uploadUrl = NULL;
                PSTR quote = NULL;
                ULONG bufferLength = 0;
                UCHAR hash[32];

                status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA256, hash);
                if (!NT_SUCCESS(status))
                {
                    RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                    __leave;
                }

                hashString = PhBufferToHexString(hash, 32);
                subObjectName = PhConcatStrings2(L"/file/upload/?sha256=", hashString->Buffer);

                if (!PerformSubRequest(context, serviceInfo->HostName, subObjectName->Buffer, &subRequestBuffer, &bufferLength))
                {
                    PhDereferenceObject(hashString);
                    PhDereferenceObject(subObjectName);
                    __leave;
                }

                if (strstr(subRequestBuffer, "\"file_exists\": true"))
                {
                    // No upload needed; show the results immediately.
                    context->LaunchCommand = PhFormatString(L"http://www.virustotal.com/file/%s/analysis/", hashString->Buffer);
                    
                    PhDereferenceObject(hashString);
                    PhDereferenceObject(subObjectName);

                    if (context->DialogHandle && context->LaunchCommand)
                    {
                        PostMessage(context->DialogHandle, UM_LAUNCH_COMMAND, 0, 0);
                        __leave;
                    }
                    else
                    {
                        RaiseUploadError(context, L"Unable to complete the LaunchCommand (please try again after a few minutes)", 0);
                        __leave;
                    }
                }

                PhDereferenceObject(hashString);
                PhDereferenceObject(subObjectName);

                uploadUrl = strstr(subRequestBuffer, "\"upload_url\": \"https://www.virustotal.com");
                if (!uploadUrl)
                {
                    RaiseUploadError(context, L"Unable to complete the request (no upload URL provided)", 0);
                    __leave;
                }

                uploadUrl += 41;
                quote = strchr(uploadUrl, '"');
                if (!quote)
                {
                    RaiseUploadError(context, L"Unable to complete the request (invalid upload URL)", 0);
                    __leave;
                }

                objectName = PhCreateStringFromAnsiEx(uploadUrl, (quote - uploadUrl) * sizeof(WCHAR));
            }
            break;
        case UPLOAD_SERVICE_JOTTI:
            {
                PPH_STRING hashString = NULL;
                PPH_STRING subObjectName = NULL;                     
                PSTR uploadId = NULL;
                PSTR quote = NULL;
                ULONG bufferLength = 0;
                UCHAR hash[20];

                status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA1, hash);
                if (!NT_SUCCESS(status))
                {
                    RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                    __leave;
                }

                hashString = PhBufferToHexString(hash, 20);
                subObjectName = PhConcatStrings2(L"/nestor/getfileforhash.php?hash=", hashString->Buffer);

                if (!PerformSubRequest(context, serviceInfo->HostName, subObjectName->Buffer, &subRequestBuffer, &bufferLength))
                {
                    PhDereferenceObject(hashString);
                    PhDereferenceObject(subObjectName);
                    __leave;
                }

                PhDereferenceObject(hashString);
                PhDereferenceObject(subObjectName);

                if (uploadId = strstr(subRequestBuffer, "\"id\":"))
                {
                    uploadId += 6;
                    quote = strchr(uploadId, '"');

                    if (quote)
                    {
                        // No upload needed; show the results immediately.
                        context->LaunchCommand = PhFormatString(L"http://virusscan.jotti.org/en/scanresult/%.*S", quote - uploadId, uploadId);

                        if (context->DialogHandle && context->LaunchCommand)
                        {
                            PostMessage(context->DialogHandle, UM_LAUNCH_COMMAND, 0, 0);
                            __leave;
                        }
                        else
                        {
                            RaiseUploadError(context, L"Unable to complete the LaunchCommand (please try again after a few minutes)", 0);
                            __leave;
                        }
                    }
                }

                objectName = PhCreateString(serviceInfo->UploadObjectName);
            }
            break;
        case UPLOAD_SERVICE_CIMA: 
            objectName = PhCreateString(serviceInfo->UploadObjectName);
            break;
        default:
            {
                RaiseUploadError(context, L"Unknown service error", 0);
                __leave;
            }
        }

        // Connect to the online service.
        if (!(connectHandle = WinHttpConnect(context->HttpHandle,
            serviceInfo->HostName,
            serviceInfo->HostPort,
            0
            )))
        {
            RaiseUploadError(context, L"Unable to connect to the service", GetLastError());
            __leave;
        }

        // Create the request.
        if (!(requestHandle = WinHttpOpenRequest(connectHandle,
            L"POST",
            objectName->Buffer,
            NULL, // HTTP/1.1
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_REFRESH | serviceInfo->HostFlags
            )))
        {
            RaiseUploadError(context, L"Unable to create the request", GetLastError());
            __leave;
        }

        // TODO? Set timeouts and disable http redirection
        //ULONG timeout = 5 * 60 * 1000; // 5 minutes
        //WinHttpSetTimeouts(requestHandle, timeout, timeout, timeout, timeout);

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
        PhAppendFormatStringBuilder(&httpRequestHeaders,
            L"Content-Type: multipart/form-data; boundary=%s\r\n",
            postBoundary->Buffer
            );
        // POST boundary header
        PhAppendFormatStringBuilder(&httpPostHeader,
            L"--%s\r\n",
            postBoundary->Buffer
            );
        PhAppendFormatStringBuilder(&httpPostHeader,
            L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",
            serviceInfo->FileNameFieldName,
            baseFileName->Buffer
            );
        PhAppendFormatStringBuilder(&httpPostHeader,
            L"Content-Type: application/octet-stream\r\n\r\n"
            );
        // POST boundary footer
        PhAppendFormatStringBuilder(&httpPostFooter,
            L"\r\n--%s--\r\n\r\n",
            postBoundary->Buffer
            );

        // add headers
        if (!WinHttpAddRequestHeaders(requestHandle,
            httpRequestHeaders.String->Buffer,
            -1L,
            WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD
            ))
        {
            RaiseUploadError(context, L"Unable to add request headers", GetLastError());
            __leave;
        }

        // All until now has been just for this; Calculate the total request length.
        totalUploadLength = (ULONG)wcslen(httpPostHeader.String->Buffer) + totalFileLength + (ULONG)wcslen(httpPostFooter.String->Buffer);

        // Send the request.
        if (!WinHttpSendRequest(requestHandle,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0,
            totalUploadLength, 0
            ))
        {
            RaiseUploadError(context, L"Unable to send the request", GetLastError());
            __leave;
        }

        // Convert to ANSI
        ansiPostData = PhCreateAnsiStringFromUnicode(httpPostHeader.String->Buffer);
        ansiFooterData = PhCreateAnsiStringFromUnicode(httpPostFooter.String->Buffer);

        // Start the clock.
        timeStart = time(NULL);
        timeTransferred = timeStart;

        // Write the header
        if (!WinHttpWriteData(
            requestHandle,
            ansiPostData->Buffer,
            ansiPostData->Length,
            &totalPostHeaderWritten
            ))
        {
            RaiseUploadError(context, L"Unable to write the post header", GetLastError());
            __leave;
        }

        assert(ansiPostData->Length == totalPostHeaderWritten);

        {
            ULONG uploadLength = 0;
            BYTE buffer[PAGE_SIZE];

            // Upload the file...
            while (TRUE)
            {
                status = NtReadFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    &buffer,
                    PAGE_SIZE,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;

                // Check bytes read.
                if (isb.Information == 0)
                    break;

                if (!WinHttpWriteData(requestHandle, buffer, (ULONG)isb.Information, &uploadLength))
                {
                    RaiseUploadError(context, L"Unable to upload the file data", GetLastError());
                    __leave;
                }

                // Zero our uploaded file buffer.
                //memset(buffer, 0, PAGE_SIZE);

                // TODO: Remove from loop and replace with callback code...
                totalReadLength += uploadLength;
                {
                    time_t time_taken = (time(NULL) - timeTransferred);
                    time_t bps = totalReadLength / (time_taken ? time_taken : 1);
                    //time_t remain = (MulDiv((INT)time_taken, totalFileLength, totalFileReadLength) - time_taken);

                    PPH_STRING TotalLength = PhFormatSize(totalFileLength, -1);
                    PPH_STRING TotalDownloadedLength = PhFormatSize(totalReadLength, -1);
                    PPH_STRING TotalSpeed = PhFormatSize(bps, -1);

                    PPH_STRING dlLengthString = PhFormatString(
                        L"%s of %s @ %s/s", 
                        TotalDownloadedLength->Buffer, 
                        TotalLength->Buffer,
                        TotalSpeed->Buffer
                        );

                    SetWindowText(context->StatusHandle, dlLengthString->Buffer);

                    PhDereferenceObject(dlLengthString);
                    PhDereferenceObject(TotalSpeed);
                    PhDereferenceObject(TotalLength);
                    PhDereferenceObject(TotalDownloadedLength);

                    // Update the progress bar position
                    PostMessage(context->ProgressHandle, PBM_SETPOS, MulDiv(100, totalReadLength, totalFileLength), 0);
                }
            }
        }

        assert(totalReadLength == totalFileLength);

        // Write the footer bytes
        if (!WinHttpWriteData(
            requestHandle,
            ansiFooterData->Buffer,
            ansiFooterData->Length,
            &totalPostFooterWritten
            ))
        {
            RaiseUploadError(context, L"Unable to write the post footer", GetLastError());
            __leave;
        }

        assert(ansiFooterData->Length == totalPostFooterWritten);

        // Wait for the send request to complete and recieve the response.
        if (!WinHttpReceiveResponse(requestHandle, NULL))
        {
            RaiseUploadError(context, L"Unable to receive the response", GetLastError());
            __leave;
        }
        else
        {
            // Handle service-specific actions.
            ULONG status = 0;
            ULONG statusLength = sizeof(statusLength);

            WinHttpQueryHeaders(
                requestHandle,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                NULL,
                &status,
                &statusLength,
                NULL
                );

            if (status == HTTP_STATUS_OK || status == HTTP_STATUS_REDIRECT)
            {
                switch (context->Service)
                {
                case UPLOAD_SERVICE_VIRUSTOTAL:
                    {
                        PVOID buffer = NULL;
                        ULONG bufferLength = 0;

                        // Use WinHttpQueryOption to obtain a buffer size.
                        if (!WinHttpQueryOption(requestHandle, WINHTTP_OPTION_URL, NULL, &bufferLength))
                        {
                            buffer = PhAllocate(bufferLength * sizeof(WCHAR)); //PhCreateStringEx(NULL, bufferLength * sizeof(WCHAR));

                            // Use WinHttpQueryOption again, this time to retrieve the URL in the new buffer
                            if (WinHttpQueryOption(requestHandle, WINHTTP_OPTION_URL, buffer, &bufferLength))
                            {
                                if (buffer)// && ((PWSTR)buffer)[0]) 
                                {
                                    // Display the retrieved URL...
                                    //context->LaunchCommand = PhFormatString(L"%s", (PWSTR)buffer);
                                    //PhFree(buffer);

                                    if (context->DialogHandle && context->LaunchCommand)
                                    {
                                        PostMessage(context->DialogHandle, UM_LAUNCH_COMMAND, 0, 0);
                                    }
                                    else
                                    {
                                        RaiseUploadError(context, L"Unable to complete the LaunchCommand (please try again after a few minutes)", 0);
                                        __leave;
                                    }
                                }
                            }
                        }
                    }
                    break;
                case UPLOAD_SERVICE_JOTTI:
                    {
                        PSTR hrefEquals = NULL;
                        PSTR quote = NULL;
                        PSTR buffer = NULL;
                        ULONG bufferLength = 0;

                        //This service returns some JavaScript that redirects the user to the new location.
                        if (!ReadRequestString(requestHandle, &buffer, &bufferLength))
                        {
                            RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                            __leave;
                        }

                        // The JavaScript looks like this: top.location.href="...";
                        hrefEquals = strstr(buffer, "href=\"");
                        if (hrefEquals)
                        {
                            hrefEquals += 6;
                            quote = strchr(hrefEquals, '"');

                            if (quote)
                            {
                                context->LaunchCommand = PhFormatString(
                                    L"http://virusscan.jotti.org%.*S",
                                    quote - hrefEquals,
                                    hrefEquals
                                    );
                            }
                        }
                        else
                        {
                            PSTR tooManyFiles = strstr(buffer, "Too many files");

                            if (tooManyFiles)
                            {
                                RaiseUploadError(
                                    context,
                                    L"Unable to scan the file:\n\n"
                                    L"Too many files have been scanned from this IP in a short period. "
                                    L"Please try again later",
                                    0
                                    );

                                __leave;
                            }
                        }

                        if (context->DialogHandle && context->LaunchCommand)
                        {
                            PostMessage(context->DialogHandle, UM_LAUNCH_COMMAND, 0, 0);
                        }
                        else
                        {
                            RaiseUploadError(context, L"Unable to complete the request (please try again after a few minutes)", 0);
                            __leave;
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

                        if (context->DialogHandle && context->LaunchCommand)
                        {
                            PostMessage(context->DialogHandle, UM_LAUNCH_COMMAND, 0, 0);
                        }
                        else
                        {
                            RaiseUploadError(context, L"Unable to complete the Launch request (please try again after a few minutes)", 0);
                            __leave;
                        }
                    }
                    break;
                }
            }
            else
            {
                RaiseUploadError(context, L"Unable to complete the request", 0);
                __leave;
            }
        }    
    }
    __finally
    {      
        if (requestHandle)
        {
            WinHttpCloseHandle(requestHandle);
        }

        if (connectHandle)
        {
            WinHttpCloseHandle(connectHandle);
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

        if (ansiFooterData)
        {
            PhReferenceObject(ansiFooterData);
        }

        if (ansiPostData)
        {
            PhReferenceObject(ansiPostData);
        }

        if (postBoundary)
        {
            PhDereferenceObject(postBoundary);
        }

        if (objectName)
        {
            PhDereferenceObject(objectName);
        }

        if (fileHandle)
        {
            NtClose(fileHandle);
        }
    }

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK UploadDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
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
        if (uMsg == WM_DESTROY)
        {
            if (!PhIsNullOrEmptyString(context->FileName))
            {
                PhDereferenceObject(context->FileName);
                context->FileName = NULL;
            }

            if (context->MessageFont)
            {
                DeleteObject(context->MessageFont);
                context->MessageFont = NULL;
            }

            if (context->HttpHandle)
            {
                WinHttpCloseHandle(context->HttpHandle);
                context->HttpHandle = NULL;
            }

            if (!PhIsNullOrEmptyString(context->LaunchCommand))
            {      
                PhDereferenceObject(context->LaunchCommand);
                context->LaunchCommand = NULL;
            }

            if (!PhIsNullOrEmptyString(context->ErrorMessage))
            {      
                PhDereferenceObject(context->LaunchCommand);
                context->LaunchCommand = NULL;
            }

            RemoveProp(hwndDlg, L"Context");
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND parentWindow = GetParent(hwndDlg);

            PhCenterWindow(hwndDlg, (IsWindowVisible(parentWindow) && !IsIconic(parentWindow)) ? parentWindow : NULL);
            
            context = (PUPLOAD_CONTEXT)lParam;
            context->StatusHandle = GetDlgItem(hwndDlg, IDC_STATUS);
            context->ProgressHandle = GetDlgItem(hwndDlg, IDC_PROGRESS1);
            context->MessageHandle = GetDlgItem(hwndDlg, IDC_MESSAGE);
            context->MessageFont = InitializeFont(context->MessageHandle);
            
            switch (context->Service)
            {
            case UPLOAD_SERVICE_VIRUSTOTAL:
                SetWindowText(hwndDlg, L"Uploading to VirtualTotal...");
                break;
            case UPLOAD_SERVICE_JOTTI:
                SetWindowText(hwndDlg, L"Uploading to Jotti...");
                break;
            case UPLOAD_SERVICE_CIMA: 
                SetWindowText(hwndDlg, L"Uploading to Comodo...");
                break;
            }

            PhQueueItemGlobalWorkQueue((PTHREAD_START_ROUTINE)UploadWorkerThreadStart, context);     
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                PostQuitMessage(0);
                break;
            }
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
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);;
        }
    case UM_LAUNCH_COMMAND:
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
            if (!PhIsNullOrEmptyString(context->ErrorMessage))
            {
                PhShowError(hwndDlg, L"%s", context->ErrorMessage->Buffer);
            }

            PostQuitMessage(0);
        }
        break;
    }

    return FALSE;
}

VOID UploadToOnlineService(
    __in PPH_STRING FileName,
    __in ULONG Service
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)PhAllocate(sizeof(UPLOAD_CONTEXT));
    memset(context, 0, sizeof(UPLOAD_CONTEXT));

    context->Service = Service;
    context->FileName = PhFormatString(L"%s", FileName->Buffer);

    PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)PhUploadToDialogThreadStart, (PVOID)context);
}