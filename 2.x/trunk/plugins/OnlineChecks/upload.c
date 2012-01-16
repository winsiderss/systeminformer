/*
 * Process Hacker Online Checks -
 *   uploader
 *
 * Copyright (C) 2010-2012 wj32
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

#include <phdk.h>
#include <wininet.h>
#include "onlnchk.h"
#include "sha256.h"
#include "resource.h"

#define HASH_SHA1 1
#define HASH_SHA256 2

#define UM_LAUNCH_COMMAND (WM_APP + 1)
#define UM_ERROR (WM_APP + 2)

typedef struct _UPLOAD_CONTEXT
{
    LONG RefCount;

    PPH_STRING FileName;
    ULONG Service;
    HWND WindowHandle;
    PH_QUEUED_LOCK Lock;
    HANDLE ThreadHandle;

    PPH_STRING LaunchCommand;
    PPH_STRING ErrorMessage;
} UPLOAD_CONTEXT, *PUPLOAD_CONTEXT;

typedef struct _SERVICE_INFO
{
    ULONG Id;
    PWSTR HostName;
    ULONG HostPort;
    ULONG HostFlags;
    PWSTR UploadObjectName;
    PSTR FileNameFieldName;
} SERVICE_INFO, *PSERVICE_INFO;

INT_PTR CALLBACK UploadDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

SERVICE_INFO UploadServiceInfo[] =
{
    { UPLOAD_SERVICE_VIRUSTOTAL, L"www.virustotal.com", INTERNET_DEFAULT_HTTPS_PORT, INTERNET_FLAG_SECURE, L"???", "file" },
    { UPLOAD_SERVICE_JOTTI, L"virusscan.jotti.org", INTERNET_DEFAULT_HTTP_PORT, 0, L"/processupload.php", "scanfile" },
    { UPLOAD_SERVICE_CIMA, L"camas.comodo.com", INTERNET_DEFAULT_HTTP_PORT, 0, L"/cgi-bin/submit", "file" }
};

PUPLOAD_CONTEXT CreateUploadContext(
    VOID
    )
{
    PUPLOAD_CONTEXT context;

    context = PhAllocate(sizeof(UPLOAD_CONTEXT));
    memset(context, 0, sizeof(UPLOAD_CONTEXT));
    context->RefCount = 1;

    return context;
}

VOID ReferenceUploadContext(
    __inout PUPLOAD_CONTEXT Context
    )
{
    _InterlockedIncrement(&Context->RefCount);
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
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROGRESS),
        hWnd,
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

    PhAcquireQueuedLockExclusive(&Context->Lock);

    if (Context->WindowHandle)
        PostMessage(Context->WindowHandle, UM_ERROR, 0, 0);

    PhReleaseQueuedLockExclusive(&Context->Lock);
}

static VOID SendLaunchCommand(
    __in PUPLOAD_CONTEXT Context
    )
{
    PhAcquireQueuedLockExclusive(&Context->Lock);

    if (Context->WindowHandle)
        PostMessage(Context->WindowHandle, UM_LAUNCH_COMMAND, 0, 0);

    PhReleaseQueuedLockExclusive(&Context->Lock);
}

static PSERVICE_INFO GetUploadServiceInfo(
    __in ULONG Id
    )
{
    ULONG i;

    for (i = 0; i < sizeof(UploadServiceInfo) / sizeof(SERVICE_INFO); i++)
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
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    )
{
    BOOLEAN result = FALSE;
    PPH_STRING userAgent;
    HINTERNET internetHandle = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;

    // Create a user agent string.
    {
        PPH_STRING phVersion;

        phVersion = PhGetPhVersion();
        userAgent = PhConcatStrings2(L"Process Hacker ", phVersion->Buffer);
        PhDereferenceObject(phVersion);
    }

    // Create the internet handle.

    internetHandle = InternetOpen(userAgent->Buffer, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    PhDereferenceObject(userAgent);

    if (!internetHandle)
    {
        RaiseUploadError(Context, L"Unable to initialize internet access", GetLastError());
        goto ExitCleanup;
    }

    // Set the timeouts.
    {
        ULONG timeout = 5 * 60 * 1000; // 5 minutes

        InternetSetOption(internetHandle, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_DATA_SEND_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, sizeof(ULONG));
    }

    // Connect to the online service.

    connectHandle = InternetConnect(
        internetHandle,
        HostName,
        80,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
        );

    if (!connectHandle)
    {
        RaiseUploadError(Context, L"Unable to connect to the service", GetLastError());
        goto ExitCleanup;
    }

    // Create the request.

    {
        static PWSTR acceptTypes[2] = { L"*/*", NULL };

        requestHandle = HttpOpenRequest(
            connectHandle,
            L"GET",
            ObjectName,
            L"HTTP/1.1",
            L"",
            acceptTypes,
            INTERNET_FLAG_RELOAD,
            0
            );
    }

    if (!requestHandle)
    {
        RaiseUploadError(Context, L"Unable to create the request", GetLastError());
        goto ExitCleanup;
    }

    // Send the request.

    if (!HttpSendRequest(requestHandle, NULL, 0, NULL, 0))
    {
        RaiseUploadError(Context, L"Unable to send the request", GetLastError());
        goto ExitCleanup;
    }

    // Handle service-specific actions.

    if (!InternetReadFile(requestHandle, Buffer, BufferLength, ReturnLength))
    {
        RaiseUploadError(Context, L"Unable to complete the request", GetLastError());
        goto ExitCleanup;
    }

    result = TRUE;

ExitCleanup:
    if (requestHandle)
        InternetCloseHandle(requestHandle);
    if (connectHandle)
        InternetCloseHandle(connectHandle);
    if (internetHandle)
        InternetCloseHandle(internetHandle);

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
    NTSTATUS status;
    PUPLOAD_CONTEXT context = Parameter;
    PSERVICE_INFO serviceInfo;
    PPH_STRING userAgent;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER fileSize64;
    ULONG fileSize;
    PPH_STRING objectName = NULL;
    HINTERNET internetHandle = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;
    PPH_STRING boundary = NULL;
    PPH_ANSI_STRING boundaryAnsi = NULL;
    PH_STRING_BUILDER headers = { 0 };
    PPH_ANSI_STRING baseFileNameAnsi = NULL;
    PUCHAR data = NULL;
    ULONG dataLength = 0;
    ULONG dataCursor = 0;
    UCHAR buffer[PAGE_SIZE];
    ULONG bufferSize;

    serviceInfo = GetUploadServiceInfo(context->Service);

    if (!serviceInfo)
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

            fileSize = fileSize64.LowPart;
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

            status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA256, hash);

            if (!NT_SUCCESS(status))
            {
                RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                goto ExitCleanup;
            }

            hashString = PhBufferToHexString(hash, 32);
            subObjectName = PhConcatStrings2(L"/file/upload/?sha256=", hashString->Buffer);

            if (!PerformSubRequest(context, serviceInfo->HostName, subObjectName->Buffer, buffer, sizeof(buffer) - 1, &bufferSize))
            {
                PhDereferenceObject(hashString);
                PhDereferenceObject(subObjectName);
                goto ExitCleanup;
            }

            PhDereferenceObject(subObjectName);

            buffer[bufferSize] = 0;

            if (strstr(buffer, "\"file_exists\": true"))
            {
                // No upload needed; show the results immediately.
                context->LaunchCommand = PhFormatString(L"http://www.virustotal.com/file/%s/analysis/", hashString->Buffer);
                PhDereferenceObject(hashString);
                SendLaunchCommand(context);
                goto ExitCleanup;
            }

            PhDereferenceObject(hashString);

            uploadUrl = strstr(buffer, "\"upload_url\": \"https://www.virustotal.com");

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
            PSTR id;
            PSTR quote;

            status = HashFileAndResetPosition(fileHandle, &fileSize64, HASH_SHA1, hash);

            if (!NT_SUCCESS(status))
            {
                RaiseUploadError(context, L"Unable to hash the file", RtlNtStatusToDosError(status));
                goto ExitCleanup;
            }

            hashString = PhBufferToHexString(hash, 20);
            subObjectName = PhConcatStrings2(L"/nestor/getfileforhash.php?hash=", hashString->Buffer);

            if (!PerformSubRequest(context, serviceInfo->HostName, subObjectName->Buffer, buffer, sizeof(buffer) - 1, &bufferSize))
            {
                PhDereferenceObject(hashString);
                PhDereferenceObject(subObjectName);
                goto ExitCleanup;
            }

            PhDereferenceObject(hashString);
            PhDereferenceObject(subObjectName);

            buffer[bufferSize] = 0;

            if (id = strstr(buffer, "\"id\":"))
            {
                id += 6;
                quote = strchr(id, '"');

                if (quote)
                {
                    // No upload needed; show the results immediately.
                    context->LaunchCommand = PhFormatString(L"http://virusscan.jotti.org/en/scanresult/%.*S", quote - id, id);
                    SendLaunchCommand(context);
                    goto ExitCleanup;
                }
            }

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
        PPH_STRING phVersion;

        phVersion = PhGetPhVersion();
        userAgent = PhConcatStrings2(L"Process Hacker ", phVersion->Buffer);
        PhDereferenceObject(phVersion);
    }

    // Create the internet handle.

    internetHandle = InternetOpen(userAgent->Buffer, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    PhDereferenceObject(userAgent);

    if (!internetHandle)
    {
        RaiseUploadError(context, L"Unable to initialize internet access", GetLastError());
        goto ExitCleanup;
    }

    // Set the timeouts.
    {
        ULONG timeout = 5 * 60 * 1000; // 5 minutes

        InternetSetOption(internetHandle, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_DATA_SEND_TIMEOUT, &timeout, sizeof(ULONG));
        InternetSetOption(internetHandle, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, sizeof(ULONG));
    }

    // Connect to the online service.

    connectHandle = InternetConnect(
        internetHandle,
        serviceInfo->HostName,
        serviceInfo->HostPort,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
        );

    if (!connectHandle)
    {
        RaiseUploadError(context, L"Unable to connect to the service", GetLastError());
        goto ExitCleanup;
    }

    // Create the request.

    {
        static PWSTR acceptTypes[2] = { L"*/*", NULL };

        requestHandle = HttpOpenRequest(
            connectHandle,
            L"POST",
            objectName->Buffer,
            L"HTTP/1.1",
            L"",
            acceptTypes,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_AUTO_REDIRECT | serviceInfo->HostFlags,
            0
            );
    }

    if (!requestHandle)
    {
        RaiseUploadError(context, L"Unable to create the request", GetLastError());
        goto ExitCleanup;
    }

    // Create the boundary.

    {
        static ULONG seed = 0;

        boundary = PhFormatString(
            L"------------------------%I64u",
            (ULONG64)RtlRandomEx(&seed) | ((ULONG64)RtlRandomEx(&seed) << 31)
            );
        boundaryAnsi = PhCreateAnsiStringFromUnicodeEx(boundary->Buffer, boundary->Length);
    }

    // Create the data.

    {
        PSTR contentDispositionPart1 = "Content-Disposition: form-data; name=\"";
        PSTR fileNameFieldName;
        PSTR contentDispositionPart2 = "\"; filename=\"";
        PSTR contentType = "Content-Type: application/octet-stream\r\n\r\n";
        PPH_STRING baseFileName;
        IO_STATUS_BLOCK isb;

        baseFileName = PhGetBaseName(context->FileName);
        baseFileNameAnsi = PhCreateAnsiStringFromUnicodeEx(baseFileName->Buffer, baseFileName->Length);
        PhDereferenceObject(baseFileName);

        fileNameFieldName = serviceInfo->FileNameFieldName;

        dataLength = 2; // --
        dataLength += boundaryAnsi->Length;
        dataLength += 2; // \r\n
        dataLength += (ULONG)strlen(contentDispositionPart1);
        dataLength += (ULONG)strlen(fileNameFieldName);
        dataLength += (ULONG)strlen(contentDispositionPart2);
        dataLength += baseFileNameAnsi->Length;
        dataLength += 3; // \"\r\n
        dataLength += (ULONG)strlen(contentType);
        dataLength += fileSize;
        dataLength += 4; // \r\n--
        dataLength += boundaryAnsi->Length;
        dataLength += 6; // --\r\n\r\n

        data = PhAllocatePage(dataLength, NULL);

        if (!data)
        {
            RaiseUploadError(context, L"Unable to allocate memory", 0);
            goto ExitCleanup;
        }

        // Boundary
        memcpy(&data[dataCursor], "--", 2);
        dataCursor += 2;
        memcpy(&data[dataCursor], boundaryAnsi->Buffer, boundaryAnsi->Length);
        dataCursor += boundaryAnsi->Length;
        memcpy(&data[dataCursor], "\r\n", 2);
        dataCursor += 2;

        // Content Disposition
        memcpy(&data[dataCursor], contentDispositionPart1, strlen(contentDispositionPart1));
        dataCursor += (ULONG)strlen(contentDispositionPart1);
        memcpy(&data[dataCursor], fileNameFieldName, strlen(fileNameFieldName));
        dataCursor += (ULONG)strlen(fileNameFieldName);
        memcpy(&data[dataCursor], contentDispositionPart2, strlen(contentDispositionPart2));
        dataCursor += (ULONG)strlen(contentDispositionPart2);
        memcpy(&data[dataCursor], baseFileNameAnsi->Buffer, baseFileNameAnsi->Length);
        dataCursor += baseFileNameAnsi->Length;
        memcpy(&data[dataCursor], "\"\r\n", 3);
        dataCursor += 3;

        // Content Type
        memcpy(&data[dataCursor], contentType, strlen(contentType));
        dataCursor += (ULONG)strlen(contentType);

        // File contents
        status = NtReadFile(fileHandle, NULL, NULL, NULL, &isb, &data[dataCursor], fileSize, NULL, NULL);
        dataCursor += fileSize;

        if (!NT_SUCCESS(status))
        {
            RaiseUploadError(context, L"Unable to read the file", RtlNtStatusToDosError(status));
            goto ExitCleanup;
        }

        // Boundary
        memcpy(&data[dataCursor], "\r\n--", 4);
        dataCursor += 4;
        memcpy(&data[dataCursor], boundaryAnsi->Buffer, boundaryAnsi->Length);
        dataCursor += boundaryAnsi->Length;
        memcpy(&data[dataCursor], "--\r\n\r\n", 6);
        dataCursor += 6;

        assert(dataCursor == dataLength);
    }

    // Create and add the header.

    {
        PhInitializeStringBuilder(&headers, 100);

        PhAppendStringBuilder2(
            &headers,
            L"Content-Type: multipart/form-data; boundary="
            );
        PhAppendStringBuilder(&headers, boundary);
        PhAppendStringBuilder2(&headers, L"\r\n");
        PhAppendFormatStringBuilder(&headers, L"Content-Length: %u\r\n", dataLength);

        HttpAddRequestHeaders(
            requestHandle,
            headers.String->Buffer,
            (ULONG)headers.String->Length,
            HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD
            );
    }

    // Send the request.

    if (!HttpSendRequest(requestHandle, NULL, 0, data, dataLength))
    {
        RaiseUploadError(context, L"Unable to send the request", GetLastError());
        goto ExitCleanup;
    }

    // Handle service-specific actions.

    {
        ULONG index;

        bufferSize = sizeof(buffer);
        index = 0;

        switch (context->Service)
        {
        case UPLOAD_SERVICE_VIRUSTOTAL:
            {
                //HttpQueryInfo(requestHandle, HTTP_QUERY_RAW_HEADERS_CRLF, buffer, &bufferSize, &index);
                //PhShowInformation(NULL, L"%.*s", bufferSize, buffer);

                if (!HttpQueryInfo(requestHandle, HTTP_QUERY_LOCATION, buffer, &bufferSize, &index))
                {
                    RaiseUploadError(context, L"Unable to complete the request (please try again after a few minutes)", GetLastError());
                    goto ExitCleanup;
                }

                if (bufferSize != 0 && *(PWCHAR)buffer == '/')
                    context->LaunchCommand = PhConcatStrings2(L"http://www.virustotal.com", (PWSTR)buffer);
                else
                    context->LaunchCommand = PhCreateString((PWSTR)buffer);
            }
            break;
        case UPLOAD_SERVICE_JOTTI:
            {
                PSTR hrefEquals;
                PSTR quote;

                // This service returns some JavaScript that redirects the user to the new location.

                if (!InternetReadFile(requestHandle, buffer, sizeof(buffer) - 1, &bufferSize))
                {
                    RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                    goto ExitCleanup;
                }

                // Make sure the buffer is null-terminated.
                buffer[bufferSize] = 0;

                // The JavaScript looks like this:
                // top.location.href="...";
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
                    PSTR tooManyFiles;

                    tooManyFiles = strstr(buffer, "Too many files");

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
                PSTR urlEquals;
                PSTR quote;

                // This service returns some HTML that redirects the user to the new location.

                if (!InternetReadFile(requestHandle, buffer, sizeof(buffer) - 1, &bufferSize))
                {
                    RaiseUploadError(context, L"Unable to complete the request", GetLastError());
                    goto ExitCleanup;
                }

                // Make sure the buffer is null-terminated.
                buffer[bufferSize] = 0;

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

        SendLaunchCommand(context);
    }

ExitCleanup:
    if (data)
        PhFreePage(data);
    if (baseFileNameAnsi)
        PhDereferenceObject(baseFileNameAnsi);
    if (headers.String)
        PhDeleteStringBuilder(&headers);
    if (boundaryAnsi)
        PhDereferenceObject(boundaryAnsi);
    if (boundary)
        PhDereferenceObject(boundary);
    if (requestHandle)
        InternetCloseHandle(requestHandle);
    if (connectHandle)
        InternetCloseHandle(connectHandle);
    if (internetHandle)
        InternetCloseHandle(internetHandle);
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
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            SetDlgItemText(hwndDlg, IDC_MESSAGE, L"Uploading...");

            context->WindowHandle = hwndDlg;

            ReferenceUploadContext(context);
            context->ThreadHandle = PhCreateThread(0, UploadWorkerThreadStart, context);
        }
        break;
    case WM_DESTROY:
        {
            PhAcquireQueuedLockExclusive(&context->Lock);
            context->WindowHandle = NULL;
            PhReleaseQueuedLockExclusive(&context->Lock);
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

