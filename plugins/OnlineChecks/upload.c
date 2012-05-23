/*
 * Process Hacker Online Checks -
 *   uploader
 *
 * Copyright (C) 2010-2012 wj32
 * Copyright (C) 2012 dmex
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

VOID InitializeFont(
    __in HWND hwndDlg
    )
{
    LOGFONT lFont = { 0 };

    // TODO: Cache font handle, it doesn't need to be recreated for each page.
    HFONT fHandle = NULL;

    lFont.lfHeight = 20;
    lFont.lfWeight = FW_MEDIUM;
    lFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;

    wcscpy_s(
        lFont.lfFaceName, 
        _countof(lFont.lfFaceName), 
        L"Segoe UI"
        );

    fHandle = CreateFontIndirectW(&lFont);

    SendMessageW(hwndDlg, WM_SETFONT, (WPARAM)fHandle, FALSE);
}

PUPLOAD_CONTEXT CreateUploadContext(
    VOID
    )
{
    PUPLOAD_CONTEXT context;

    context = PhAllocate(sizeof(UPLOAD_CONTEXT));
    ZeroMemory(context, sizeof(UPLOAD_CONTEXT));
    context->RefCount = 1;

    return context;
}

VOID DereferenceUploadContext(
    __inout PUPLOAD_CONTEXT Context
    )
{
    if (_InterlockedDecrement(&Context->RefCount) == 0)
    {
        PhSwapReference(&Context->FileName, NULL);
        PhSwapReference(&Context->LaunchCommand, NULL);
        PhSwapReference(&Context->ErrorMessage, NULL);

        PhFree(Context);
    }
}

VOID UploadToOnlineService(
    __in HWND hWnd,
    __in PPH_STRING FileName,
    __in ULONG Service
    )
{
    PUPLOAD_CONTEXT context;

    context = CreateUploadContext();

    PhSwapReference(&context->FileName, FileName);
    context->Service = Service;

    DialogBoxParam(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROGRESSDIALOG),
        NULL,
        UploadDlgProc,
        (LPARAM)context
        );

    DereferenceUploadContext(context);
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

    if (Context->WindowHandle)
        PostMessage(Context->WindowHandle, UM_ERROR, 0, 0);
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
    __out_bcount(DataLength) PSTR *Data,
    __out_opt PULONG DataLength
    )
{
    BOOLEAN result = FALSE;
    PPH_STRING userAgent;
    HINTERNET internetHandle = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;

    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxycfg = { 0 };
    // Create a user agent string.
    {
        PPH_STRING phVersion = PhGetPhVersion();
        userAgent = PhConcatStrings2(L"Process Hacker ", phVersion->Buffer);
        PhDereferenceObject(phVersion);
    }

    if (!WinHttpGetIEProxyConfigForCurrentUser(&proxycfg))
    {
        RaiseUploadError(Context, L"WinHttpGetIEProxyConfigForCurrentUser Failure", GetLastError());
        goto ExitCleanup;
    }

    // Create the internet handle.
    if (!(internetHandle = WinHttpOpen(
        userAgent->Buffer, 
        proxycfg.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxycfg.lpszProxy, 
        proxycfg.lpszProxyBypass, 
        0
        )))
    {
        RaiseUploadError(Context, L"Unable to initialize internet access", GetLastError());
        goto ExitCleanup;
    }

    PhDereferenceObject(userAgent);

    // Connect to the online service.
    if (!(connectHandle = WinHttpConnect(
        internetHandle,
        HostName,
        INTERNET_DEFAULT_HTTP_PORT,
        0
        )))
    {
        RaiseUploadError(Context, L"Unable to connect to the service", GetLastError());
        goto ExitCleanup;
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
        goto ExitCleanup;
    }

    // Send the request.
    if (!WinHttpSendRequest(requestHandle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        RaiseUploadError(Context, L"Unable to send the request", GetLastError());
        goto ExitCleanup;
    }

    // Wait for the send request to complete and recieve the response.
    if (!WinHttpReceiveResponse(requestHandle, NULL))
    {
        RaiseUploadError(Context, L"Unable to receive the response", GetLastError());
        goto ExitCleanup;
    }

    // Handle service-specific actions.
    //if (!WinHttpReadData(requestHandle, Buffer, BufferLength, ReturnLength))
    //{
    //    RaiseUploadError(Context, L"Unable to complete the request", GetLastError());
    //    goto ExitCleanup;
    //}

    {
        BYTE buffer[BUFFER_LEN];
        PSTR data;
        ULONG allocatedLength;
        ULONG dataLength;
        ULONG returnLength;

        allocatedLength = sizeof(buffer);
        data = PhAllocate(allocatedLength);
        dataLength = 0;

        while (WinHttpReadData(requestHandle, buffer, BUFFER_LEN, &returnLength))
        {
            if (returnLength == 0)
                break;

            if (allocatedLength < dataLength + returnLength)
            {
                allocatedLength *= 2;
                data = PhReAllocate(data, allocatedLength);
            }

            RtlCopyMemory(data + dataLength, buffer, returnLength);
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
            *DataLength = dataLength;

    }

    result = TRUE;

ExitCleanup:
    if (requestHandle)
        WinHttpCloseHandle(requestHandle);
    if (connectHandle)
        WinHttpCloseHandle(connectHandle);
    if (internetHandle)
        WinHttpCloseHandle(internetHandle);

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
    __in PVOID Parameter
    )
{
    ULONG seed = 0;
    NTSTATUS status;
    LARGE_INTEGER fileSize64;

    time_t TimeStart = 0, TimeTransferred = 0;
    DWORD totalFileLength = 0, totalFileReadLength = 0;

    HANDLE fileHandle = NULL;
    HINTERNET internetHandle = NULL, connectHandle = NULL, requestHandle = NULL;
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)Parameter;
    PSERVICE_INFO serviceInfo = NULL;

    PPH_STRING userAgent = NULL, objectName = NULL, baseFileName = PhGetBaseName(context->FileName);
   
    if (!(serviceInfo = GetUploadServiceInfo(context->Service)))
    {
        RaiseUploadError(context, L"The service type is invalid", 0);
        goto ExitCleanup;
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
                goto ExitCleanup;
            }

            totalFileLength = fileSize64.LowPart;
        }
    }

    if (!NT_SUCCESS(status))
    {
        RaiseUploadError(context, L"Unable to open the file", RtlNtStatusToDosError(status));
        goto ExitCleanup;
    }

    switch (context->Service)
    {
    case UPLOAD_SERVICE_VIRUSTOTAL:
        {
            UCHAR hash[32];
            PPH_STRING hashString;
            PPH_STRING subObjectName;
            PSTR uploadUrl;
            PSTR quote;
            PSTR data;
            DWORD length;

            status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA256, hash);

            if (!NT_SUCCESS(status))
            {
                RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                goto ExitCleanup;
            }

            hashString = PhBufferToHexString(hash, 32);
            subObjectName = PhConcatStrings2(L"/file/upload/?sha256=", hashString->Buffer);

            if (!PerformSubRequest(context, serviceInfo->HostName, subObjectName->Buffer, &data, &length))
            {
                PhDereferenceObject(hashString);
                PhDereferenceObject(subObjectName);
                goto ExitCleanup;
            }

            //if (strstr(buffer, "\"file_exists\": true"))
            //{
            //    // No upload needed; show the results immediately.
            //    context->LaunchCommand = PhFormatString(L"http://www.virustotal.com/file/%s/analysis/", hashString->Buffer);
            //    PhDereferenceObject(hashString);
            //    SendLaunchCommand(context);
            //    goto ExitCleanup;
            //}

            PhDereferenceObject(hashString);
            PhDereferenceObject(subObjectName);

            uploadUrl = strstr(data, "\"upload_url\": \"https://www.virustotal.com");

            if (!uploadUrl)
            {
                RaiseUploadError(context, L"Unable to complete the request (no upload URL provided)", 0);
                goto ExitCleanup;
            }

            uploadUrl += 41;
            quote = strchr(uploadUrl, '"');

            if (!quote)
            {
                RaiseUploadError(context, L"Unable to complete the request (invalid upload URL)", 0);
                goto ExitCleanup;
            }

            objectName = PhCreateStringFromAnsiEx(uploadUrl, quote - uploadUrl);
        }
        break;
    case UPLOAD_SERVICE_JOTTI:
        {
            UCHAR hash[20];
            PPH_STRING hashString;
            PPH_STRING subObjectName;
            PSTR data;
            DWORD length;

            status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA1, hash);

            if (!NT_SUCCESS(status))
            {
                RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                goto ExitCleanup;
            }

            hashString = PhBufferToHexString(hash, 20);
            subObjectName = PhConcatStrings2(L"/nestor/getfileforhash.php?hash=", hashString->Buffer);

            if (!PerformSubRequest(context, serviceInfo->HostName, subObjectName->Buffer, &data, &length))
            {
                PhDereferenceObject(hashString);
                PhDereferenceObject(subObjectName);
                goto ExitCleanup;
            }

            PhDereferenceObject(hashString);
            PhDereferenceObject(subObjectName);

            //if (id = strstr(data, "\"id\":"))
            //{
            //    id += 6;
            //    quote = strchr(id, '"');

            //    if (quote)
            //    {
            //        // No upload needed; show the results immediately.
            //        context->LaunchCommand = PhFormatString(L"http://virusscan.jotti.org/en/scanresult/%.*S", quote - id, id);
            //        SendLaunchCommand(context);
            //        goto ExitCleanup;
            //    }
            //}

            objectName = PhCreateString(serviceInfo->UploadObjectName);
        }
        break;
    default:
        {
            objectName = PhCreateString(serviceInfo->UploadObjectName);
        }
        break;
    }

    // Create a user agent string.
    {
        PPH_STRING phVersion = PhGetPhVersion();
        userAgent = PhConcatStrings2(L"Process Hacker ", phVersion->Buffer);
        PhDereferenceObject(phVersion);
    }
    
    // Get proxy configuration and create winhttp handle. 
    {
        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxycfg = { 0 };

        if (!WinHttpGetIEProxyConfigForCurrentUser(&proxycfg))
        {
            RaiseUploadError(context, L"WinHttpGetIEProxyConfigForCurrentUser Failure", GetLastError());
            goto ExitCleanup;
        }

        // Create the internet handle.
        if (!(internetHandle = WinHttpOpen(
            userAgent->Buffer, 
            proxycfg.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
            proxycfg.lpszProxy, 
            proxycfg.lpszProxyBypass, 
            0
            )))
        {
            RaiseUploadError(context, L"Unable to initialize internet access", GetLastError());
            goto ExitCleanup;
        }
    }

    // Connect to the online service.
    if (!(connectHandle = WinHttpConnect(
        internetHandle,
        serviceInfo->HostName,
        serviceInfo->HostPort,
        0
        )))
    {
        RaiseUploadError(context, L"Unable to connect to the service", GetLastError());
        goto ExitCleanup;
    }

    // Create the request.
    if (!(requestHandle = WinHttpOpenRequest(
        connectHandle,
        L"POST",
        objectName->Buffer,
        NULL, // HTTP/1.1
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | serviceInfo->HostFlags
        )))
    {
        RaiseUploadError(context, L"Unable to create the request", GetLastError());
        goto ExitCleanup;
    }

    // Set timeouts and disable http redirection.
    {
        DWORD disable = WINHTTP_DISABLE_REDIRECTS;
        ULONG timeout = 5 * 60 * 1000; // 5 minutes

        WinHttpSetOption(requestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &disable, sizeof(ULONG));
        WinHttpSetOption(requestHandle, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(ULONG));
        WinHttpSetOption(requestHandle, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(ULONG));
        WinHttpSetOption(requestHandle, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(ULONG));
    }
    
    // Create and POST data.
    {
        PH_STRING_BUILDER sbRequestHeaders = { 0 };
        PH_STRING_BUILDER sbPostHeader = { 0 };
        PH_STRING_BUILDER sbPostFooter = { 0 };

        PPH_STRING postBoundary = PhFormatString(
            L"------------------------%I64u",
            (ULONG64)RtlRandomEx(&seed) | ((ULONG64)RtlRandomEx(&seed) << 31)
            );

        PhInitializeStringBuilder(&sbRequestHeaders, MAX_PATH);
        PhInitializeStringBuilder(&sbPostHeader, MAX_PATH);
        PhInitializeStringBuilder(&sbPostFooter, MAX_PATH);
  
        // build request header string
        PhAppendFormatStringBuilder(
            &sbRequestHeaders,
            L"Content-Type: multipart/form-data; boundary=%s\r\n",
            postBoundary->Buffer
            );
        // POST boundary header
        PhAppendFormatStringBuilder(
            &sbPostHeader,
            L"--%s\r\n",
            postBoundary->Buffer
            );
        PhAppendFormatStringBuilder(
            &sbPostHeader,
            L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n",
            serviceInfo->FileNameFieldName,
            baseFileName->Buffer
            );
        PhAppendFormatStringBuilder(
            &sbPostHeader, 
            L"Content-Type: application/octet-stream\r\n\r\n"
            );
        // POST boundary footer  
        PhAppendFormatStringBuilder(
            &sbPostFooter,
            L"\r\n--%s--\r\n\r\n",
            postBoundary->Buffer
            );
  
        // add headers
        if (!WinHttpAddRequestHeaders(
            requestHandle,
            sbRequestHeaders.String->Buffer,
            -1,
            WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD
            ))
        {
            RaiseUploadError(context, L"Unable to add request headers", GetLastError());
            goto ExitCleanup;
        }

        {
            // All until now has been just for this; Calculate the total request length.
            ULONG total_length = 
                (ULONG)wcsnlen(sbPostHeader.String->Buffer, sbPostHeader.String->Length) 
                + totalFileLength 
                + (ULONG)wcsnlen(sbPostFooter.String->Buffer, sbPostFooter.String->Length);

            // Send the request.
            if (!WinHttpSendRequest(
                requestHandle,
                WINHTTP_NO_ADDITIONAL_HEADERS, 
                0, 
                WINHTTP_NO_REQUEST_DATA, 
                0, 
                total_length, 
                0
                ))
            {
                RaiseUploadError(context, L"Unable to send the request", GetLastError());
                goto ExitCleanup;
            }
        }

        {
            DWORD bytesRead = 0, bytesWritten = 0;
            IO_STATUS_BLOCK isb;
            BYTE buffer[BUFFER_LEN];

            // Convert to ANSI
            PPH_ANSI_STRING ansiPostData = PhCreateAnsiStringFromUnicode(sbPostHeader.String->Buffer);
            PPH_ANSI_STRING ansiFooterData = PhCreateAnsiStringFromUnicode(sbPostFooter.String->Buffer);
 
            // start the clock.
            TimeStart = time(NULL);
            TimeTransferred = TimeStart;

            // Write the header bytes
            if (!WinHttpWriteData(
                requestHandle, 
                ansiPostData->Buffer, 
                ansiPostData->Length, 
                &bytesWritten
                ))
            {       
                RaiseUploadError(context, L"Unable to write the post header", GetLastError());
                goto ExitCleanup;
            }

            assert(ansiPostData->Length == bytesWritten);

            // Write the file bytes
            while (TRUE)
            {
                status = NtReadFile(
                    fileHandle, 
                    NULL, 
                    NULL, 
                    NULL, 
                    &isb, 
                    &buffer, 
                    BUFFER_LEN, 
                    NULL, 
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;

                // Check bytes read.
                if (isb.Information == 0) 
                    break;

                if (!WinHttpWriteData(
                    requestHandle, 
                    buffer, 
                    (DWORD)isb.Information, 
                    &bytesWritten
                    ))
                {       
                    RaiseUploadError(context, L"Unable to upload the file data", GetLastError());
                    goto ExitCleanup;
                }

                totalFileReadLength += bytesWritten;

                // Zero our upload buffer.
                ZeroMemory(buffer, BUFFER_LEN);

                //Update the GUI progress.
                {
                    WCHAR szDownloaded[1024] = L"";
                    WCHAR *rtext = L"second";
                    time_t time_taken = (time(NULL) - TimeTransferred);
                    time_t bps = totalFileReadLength / (time_taken ? time_taken : 1);
                    time_t remain = (MulDiv((INT)time_taken, totalFileLength, totalFileReadLength) - time_taken);

                    PPH_STRING dlRemaningBytes = PhFormatSize(totalFileReadLength, -1);
                    PPH_STRING dlLength = PhFormatSize(totalFileLength, -1);
                    PPH_STRING dlSpeed = PhFormatSize(bps, -1); 

                    if (remain < 0) 
                        remain = 0;
                    if (remain >= 60)
                    {
                        remain /= 60;
                        rtext = L"minute";
                        if (remain >= 60)
                        {
                            remain /= 60;
                            rtext = L"hour";
                        }
                    }

                    wsprintfW(
                        szDownloaded,
                        L"%s (%d%%) of %s @ %s/s",
                        dlRemaningBytes->Buffer,
                        MulDiv(100, totalFileReadLength, totalFileLength),
                        dlLength->Buffer,
                        dlSpeed->Buffer
                        );

                    if (remain) 
                    {
                        wsprintfW(
                            szDownloaded + lstrlenW(szDownloaded),
                            L" (%d %s%s remaining)",
                            (DWORD)remain, // Cast and silence code analysis.
                            rtext,
                            remain == 1? L"": L"s"
                            );
                    }

                    SetWindowText(GetDlgItem(context->WindowHandle, IDC_STATUS), szDownloaded);

                    PhDereferenceObject(dlSpeed);
                    PhDereferenceObject(dlLength);
                    PhDereferenceObject(dlRemaningBytes);

                    // Update the progress bar position
                    SendDlgItemMessage(context->WindowHandle, IDC_UPLOADPROGRESS, PBM_SETPOS, MulDiv(100, totalFileReadLength, totalFileLength), 0);  
                }
            }

            assert(totalFileReadLength == totalFileLength);

            // Write the footer bytes
            if (!WinHttpWriteData(
                requestHandle, 
                ansiFooterData->Buffer, 
                ansiFooterData->Length, 
                &bytesWritten
                ))
            {       
                RaiseUploadError(context, L"Unable to write the post footer", GetLastError());
                goto ExitCleanup;
            }

            assert(ansiFooterData->Length == bytesWritten);

            PhReferenceObject(ansiFooterData);
            PhReferenceObject(ansiPostData);
        }

        PhDereferenceObject(postBoundary);
        PhDeleteStringBuilder(&sbPostHeader);
        PhDeleteStringBuilder(&sbPostFooter);
    }

    // Wait for the send request to complete and recieve the response.
    if (!WinHttpReceiveResponse(requestHandle, NULL))
    {
        RaiseUploadError(context, L"Unable to receive the response", GetLastError());
        goto ExitCleanup;
    }

    // Handle service-specific actions.
    {
        DWORD dwStatusCode = 0;
        DWORD dwTemp = sizeof(dwStatusCode);

        WinHttpQueryHeaders(
            requestHandle, 
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            NULL, 
            &dwStatusCode, 
            &dwTemp, 
            NULL
            );

        if (dwStatusCode == HTTP_STATUS_OK || dwStatusCode == HTTP_STATUS_REDIRECT)
        {           
            switch (context->Service)
            {
            case UPLOAD_SERVICE_VIRUSTOTAL:
                {
                    ULONG index = 0;
                    PBYTE buffer = NULL;
                    ULONG bufferSize = 0;

                    WinHttpQueryHeaders(
                        requestHandle, 
                        WINHTTP_QUERY_LOCATION, 
                        WINHTTP_HEADER_NAME_BY_INDEX, 
                        NULL, 
                        &bufferSize, 
                        &index
                        );

                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        buffer = PhAllocate(bufferSize);

                        // Now, use WinHttpQueryHeaders to retrieve the header.
                        if (!WinHttpQueryHeaders( 
                            requestHandle,
                            WINHTTP_QUERY_LOCATION,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            buffer, 
                            &bufferSize,
                            &index
                            ))
                        {
                            RaiseUploadError(context, L"Unable to complete the request (please try again after a few minutes)", GetLastError());
                            goto ExitCleanup;
                        }
                    }

                    if (bufferSize != 0 && buffer && *(PWCHAR)buffer == '/')
                    {
                        context->LaunchCommand = PhConcatStrings2(L"http://www.virustotal.com", (PWSTR)buffer);
                    }
                    else
                    {
                        context->LaunchCommand = PhCreateString((PWSTR)buffer);
                    }

                    if (buffer)
                    {
                        PhFree(buffer);
                    }
                }
                break;
            case UPLOAD_SERVICE_JOTTI:
                {
                    PSTR hrefEquals;
                    PSTR quote;
                    DWORD bytesRead = 0;
                    BYTE buffer[512];
  
                    //This service returns some JavaScript that redirects the user to the new location.
                    if (!WinHttpReadData(requestHandle, buffer, 512, &bytesRead))
                    {
                        RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                        goto ExitCleanup;
                    }

                    //Make sure the buffer is null-terminated.
                    buffer[bytesRead] = 0;
                    /*             
                    The JavaScript looks like this:
                    top.location.href="...";*/
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

                            goto ExitCleanup;
                        }
                    }

                    if (!context->LaunchCommand)
                    {
                        RaiseUploadError(context, L"Unable to complete the request (please try again after a few minutes)", 0);
                        goto ExitCleanup;
                    }
                }
                break;
            case UPLOAD_SERVICE_CIMA:
                {              
                    BYTE buffer[512];
                    DWORD bytesRead = 0;
                    PSTR urlEquals;
                    PSTR quote;

                    // This service returns some HTML that redirects the user to the new location.

                    if (!WinHttpReadData(requestHandle, buffer, 512, &bytesRead))
                    {
                        RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                        goto ExitCleanup;
                    }

                    // Make sure the buffer is null-terminated.
                    buffer[bytesRead] = 0;

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

                    if (!context->LaunchCommand)
                    {
                        RaiseUploadError(context, L"Unable to complete the request (please try again after a few minutes)", 0);
                        goto ExitCleanup;
                    }
                }
                break;
            default:
                {
                    RaiseUploadError(context, L"Unable to complete the request", 0);
                    goto ExitCleanup;
                }
                break;
            }

            if (context->WindowHandle)
            {
                PostMessage(context->WindowHandle, UM_LAUNCH_COMMAND, 0, 0);
            }
        }
        else
        {
            RaiseUploadError(context, L"Unable to complete the request", dwStatusCode);
            goto ExitCleanup;
        }
    }

ExitCleanup:

    if (requestHandle)
        WinHttpCloseHandle(requestHandle);
    if (connectHandle)
        WinHttpCloseHandle(connectHandle);
    if (internetHandle)
        WinHttpCloseHandle(internetHandle);
    if (objectName)
        PhDereferenceObject(objectName);
    if (fileHandle)
        NtClose(fileHandle);

    DereferenceUploadContext(context);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK UploadDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PUPLOAD_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PUPLOAD_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PUPLOAD_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            _InterlockedIncrement(&context->RefCount);

            context->WindowHandle = hwndDlg;

            // Center the update window on PH if visible and not mimimized else center on desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(GetParent(hwndDlg)) && !IsIconic(GetParent(hwndDlg))) ? GetParent(hwndDlg) : NULL);

            InitializeFont(GetDlgItem(hwndDlg, IDC_MESSAGE));


            if (context->FileName)
            {
                WCHAR szWindowText[MAX_PATH];
                PPH_STRING baseFileName = PhGetBaseName(context->FileName);
                swprintf_s(
                    szWindowText, 
                    _countof(szWindowText), 
                    L"Uploading: %s", 
                    baseFileName->Buffer
                    );

                SetDlgItemText(hwndDlg, IDC_MESSAGE, szWindowText);
                PhDereferenceObject(baseFileName);

                context->ThreadHandle = PhCreateThread(0, UploadWorkerThreadStart, context);
            }
        }
        break;
    case WM_DESTROY:
        {
            context->WindowHandle = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
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
            PhShellExecute(hwndDlg, context->LaunchCommand->Buffer, NULL);
            EndDialog(hwndDlg, IDOK);
        }
        break;
    case UM_ERROR:
        {
            PhShowError(hwndDlg, L"%s", context->ErrorMessage->Buffer);
            EndDialog(hwndDlg, IDCANCEL);
        }
        break;
    }

    return FALSE;
}

