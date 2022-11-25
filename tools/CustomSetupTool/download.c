/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include "setup.h"

PPH_STRING SetupGetVersion(
    VOID
)
{
    PH_FORMAT format[7];

    PhInitFormatU(&format[0], PHAPP_VERSION_MAJOR);
    PhInitFormatC(&format[1], '.');
    PhInitFormatU(&format[2], PHAPP_VERSION_MINOR);
    PhInitFormatC(&format[3], '.');
    PhInitFormatU(&format[4], PHAPP_VERSION_REVISION);
    PhInitFormatC(&format[5], '.');
    PhInitFormatU(&format[6], PHAPP_VERSION_BUILD);

    return PhFormat(format, 7, 16);
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    PPH_STRING currentVersion = NULL;
    PPH_STRING versionHeader = NULL;

    if (currentVersion = SetupGetVersion())
    {
        versionHeader = PhConcatStrings2(L"SI-SETUP-BUILD: ", currentVersion->Buffer);
        PhDereferenceObject(currentVersion);
    }

    return versionHeader;
}

PPH_STRING UpdateWindowsString(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion");
    HANDLE keyHandle;
    PPH_STRING buildLabHeader = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        PPH_STRING buildLabString;

        if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLabEx"))
        {
            buildLabHeader = PhConcatStrings2(L"SI-OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }
        else if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLab"))
        {
            buildLabHeader = PhConcatStrings2(L"SI-OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }

        NtClose(keyHandle);
    }

    return buildLabHeader;
}

ULONG64 ParseVersionString(
    _Inout_ PPH_STRING VersionString
    )
{
    PH_STRINGREF remaining, majorPart, minorPart, revisionPart;
    ULONG64 majorInteger = 0, minorInteger = 0, revisionInteger = 0;

    PhInitializeStringRef(&remaining, PhGetStringOrEmpty(VersionString));
    PhSplitStringRefAtChar(&remaining, '.', &majorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, '.', &minorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, '.', &revisionPart, &remaining);

    PhStringToInteger64(&majorPart, 10, &majorInteger);
    PhStringToInteger64(&minorPart, 10, &minorInteger);
    PhStringToInteger64(&revisionPart, 10, &revisionInteger);

    return MAKE_VERSION_ULONGLONG(
        (ULONG)majorInteger,
        (ULONG)minorInteger,
        (ULONG)revisionInteger,
        0
        );
}

BOOLEAN ReadRequestString(
    _In_ HINTERNET Handle,
    _Out_ _Deref_post_z_cap_(*DataLength) PSTR *Data,
    _Out_ ULONG *DataLength
)
{
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    BYTE buffer[PAGE_SIZE];

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    memset(buffer, 0, PAGE_SIZE);
    memset(data, 0, allocatedLength);

    while (WinHttpReadData(Handle, buffer, PAGE_SIZE, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        memcpy(data + dataLength, buffer, returnLength);

        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    data[dataLength] = 0;

    *DataLength = dataLength;
    *Data = data;

    return TRUE;
}

BOOLEAN SetupQueryUpdateData(
    _Inout_ PPH_SETUP_CONTEXT Context
    )
{
    BOOLEAN success = FALSE;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    ULONG stringBufferLength = 0;
    PSTR stringBuffer = NULL;
    PVOID jsonObject = NULL;
    PPH_STRING versionHeader = UpdateVersionString();
    PPH_STRING windowsHeader = UpdateWindowsString();
    PSTR value = NULL;

    if (!(httpSessionHandle = WinHttpOpen(
        NULL,
        WindowsVersion >= WINDOWS_8_1 ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        WinHttpSetOption(
            httpSessionHandle,
            WINHTTP_OPTION_DECOMPRESSION,
            &(ULONG) { WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE },
            sizeof(ULONG)
            );
    }

    if (!(httpConnectionHandle = WinHttpConnect(
        httpSessionHandle,
        L"systeminformer.sourceforge.io",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        //Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(httpRequestHandle = WinHttpOpenRequest(
        httpConnectionHandle,
        NULL,
        L"/update.php?setup",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE
        )))
    {
        //Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_7)
    {
        WinHttpSetOption(
            httpRequestHandle,
            WINHTTP_OPTION_DISABLE_FEATURE,
            &(ULONG) { WINHTTP_DISABLE_KEEP_ALIVE },
            sizeof(ULONG)
            );
    }

    if (versionHeader)
    {
        WinHttpAddRequestHeaders(
            httpRequestHandle,
            versionHeader->Buffer,
            (ULONG)versionHeader->Length / sizeof(WCHAR),
            WINHTTP_ADDREQ_FLAG_ADD
            );
    }

    if (windowsHeader)
    {
        WinHttpAddRequestHeaders(
            httpRequestHandle,
            windowsHeader->Buffer,
            (ULONG)windowsHeader->Length / sizeof(WCHAR),
            WINHTTP_ADDREQ_FLAG_ADD
            );
    }

    if (!WinHttpSendRequest(
        httpRequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
        0
        ))
    {
        //Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
    {
        //Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!ReadRequestString(httpRequestHandle, &stringBuffer, &stringBufferLength))
        goto CleanupExit;

    // Check the buffer for valid data
    if (stringBuffer == NULL || stringBuffer[0] == '\0')
        goto CleanupExit;

    if (!(jsonObject = PhCreateJsonParser(stringBuffer)))
        goto CleanupExit;

    Context->RelVersion = PhGetJsonValueAsString(jsonObject, "version");
    Context->RelDate = PhGetJsonValueAsString(jsonObject, "updated");

    Context->BinFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "bin_url");
    Context->BinFileLength = PhGetJsonValueAsString(jsonObject, "bin_length");
    Context->BinFileHash = PhGetJsonValueAsString(jsonObject, "bin_hash");
    Context->BinFileSignature = PhGetJsonValueAsString(jsonObject, "bin_sig");

    Context->SetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "setup_url");
    Context->SetupFileLength = PhGetJsonValueAsString(jsonObject, "setup_length");
    Context->SetupFileHash = PhGetJsonValueAsString(jsonObject, "setup_hash");
    Context->SetupFileSignature = PhGetJsonValueAsString(jsonObject, "setup_sig");

    Context->WebSetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "websetup_url");
    Context->WebSetupFileVersion = PhGetJsonValueAsString(jsonObject, "websetup_version");
    Context->WebSetupFileLength = PhGetJsonValueAsString(jsonObject, "websetup_length");
    Context->WebSetupFileHash = PhGetJsonValueAsString(jsonObject, "websetup_hash");
    Context->WebSetupFileSignature = PhGetJsonValueAsString(jsonObject, "websetup_sig");

    if (PhIsNullOrEmptyString(Context->RelVersion))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->RelDate))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(Context->BinFileDownloadUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->BinFileSignature))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(Context->SetupFileDownloadUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileSignature))
        goto CleanupExit;

    //if (PhIsNullOrEmptyString(Context->WebSetupFileDownloadUrl))
    //    goto CleanupExit;
    //if (PhIsNullOrEmptyString(Context->WebSetupFileSignature))
    //    goto CleanupExit;
    //if (PhIsNullOrEmptyString(Context->WebSetupFileVersion))
    //    goto CleanupExit;

    success = TRUE;

CleanupExit:

    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);

    if (httpConnectionHandle)
        WinHttpCloseHandle(httpConnectionHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);

    if (jsonObject)
        PhFreeJsonObject(jsonObject);

    if (stringBuffer)
        PhFree(stringBuffer);

    PhClearReference(&versionHeader);
    PhClearReference(&windowsHeader);

    return success;
}

BOOLEAN UpdateDownloadUpdateData(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    BOOLEAN downloadSuccess = FALSE;
    HANDLE tempFileHandle = NULL;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    PPH_STRING downloadFileName = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PPH_STRING userAgentString = NULL;
    ULONG indexOfFileName = -1;
    URL_COMPONENTS httpUrlComponents = { sizeof(URL_COMPONENTS) };
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    userAgentString = PhFormatString(
        L"PH_%lu.%lu_%lu",
        Context->CurrentMajorVersion,
        Context->CurrentMinorVersion,
        Context->CurrentRevisionVersion
        );

    httpUrlComponents.dwSchemeLength = (ULONG)-1;
    httpUrlComponents.dwHostNameLength = (ULONG)-1;
    httpUrlComponents.dwUrlPathLength = (ULONG)-1;

    if (!WinHttpCrackUrl(
        PhGetString(Context->BinFileDownloadUrl),
        0,
        0,
        &httpUrlComponents
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    downloadHostPath = PhCreateStringEx(
        httpUrlComponents.lpszHostName,
        httpUrlComponents.dwHostNameLength * sizeof(WCHAR)
        );
    downloadUrlPath = PhCreateStringEx(
        httpUrlComponents.lpszUrlPath,
        httpUrlComponents.dwUrlPathLength * sizeof(WCHAR)
        );

    {
        PH_STRINGREF pathPart;
        PH_STRINGREF baseNamePart;

        if (!PhSplitStringRefAtLastChar(&downloadUrlPath->sr, '/', &pathPart, &baseNamePart))
            goto CleanupExit;

        downloadFileName = PhCreateString2(&baseNamePart);
    }

    Context->FilePath = PhCreateCacheFile(downloadFileName);

    if (PhIsNullOrEmptyString(Context->FilePath))
        goto CleanupExit;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &tempFileHandle,
        PhGetString(Context->FilePath),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhFormatString(
        L"Downloading System Informer %s...",
        PhGetString(Context->RelVersion)
        )->Buffer);

    //SetWindowText(Context->SubHeaderHandle, L"Progress: ~ of ~ (0.0%)");
    //SendMessage(Context->ProgressHandle, PBM_SETRANGE32, 0, (LPARAM)ExtractTotalLength);

    if (!(httpSessionHandle = WinHttpOpen(
        PhGetString(userAgentString),
        WindowsVersion >= WINDOWS_8_1 ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        )))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        WinHttpSetOption(
            httpSessionHandle,
            WINHTTP_OPTION_DECOMPRESSION,
            &(ULONG) { WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE },
            sizeof(ULONG)
            );
    }

    if (!(httpConnectionHandle = WinHttpConnect(
        httpSessionHandle,
        PhGetString(downloadHostPath),
        httpUrlComponents.nScheme == INTERNET_SCHEME_HTTP ? INTERNET_DEFAULT_HTTP_PORT : INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(httpRequestHandle = WinHttpOpenRequest(
        httpConnectionHandle,
        NULL,
        PhGetString(downloadUrlPath),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | (httpUrlComponents.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
        )))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_7)
    {
        WinHttpSetOption(
            httpRequestHandle,
            WINHTTP_OPTION_DISABLE_FEATURE,
            &(ULONG) { WINHTTP_DISABLE_KEEP_ALIVE },
            sizeof(ULONG)
            );
    }

    if (!WinHttpSendRequest(
        httpRequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
        0
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }
    else
    {
        ULONG bytesDownloaded = 0;
        ULONG downloadedBytes = 0;
        ULONG contentLengthSize = sizeof(ULONG);
        ULONG contentLength = 0;
        IO_STATUS_BLOCK isb;
        BYTE buffer[PAGE_SIZE];

        //SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);

        PhQuerySystemTime(&timeStart);

        if (!WinHttpQueryHeaders(
            httpRequestHandle,
            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &contentLength,
            &contentLengthSize,
            0
            ))
        {
            Context->ErrorCode = GetLastError();
            goto CleanupExit;
        }

        memset(buffer, 0, PAGE_SIZE);

        while (WinHttpReadData(httpRequestHandle, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            if (bytesDownloaded == 0)
                break;

            if (!NT_SUCCESS(NtWriteFile(
                tempFileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                bytesDownloaded,
                NULL,
                NULL
                )))
            {
                goto CleanupExit;
            }

            downloadedBytes += (ULONG)isb.Information;

            if (bytesDownloaded != isb.Information)
                goto CleanupExit;

            PhQuerySystemTime(&timeNow);

            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
            timeBitsPerSecond = downloadedBytes / __max(timeTicks, 1);

            {
                FLOAT percent = ((FLOAT)downloadedBytes / contentLength * 100);
                PH_FORMAT format[9];
                WCHAR string[MAX_PATH];

                // L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s"
                PhInitFormatS(&format[0], L"Downloaded: ");
                PhInitFormatSize(&format[1], downloadedBytes);
                PhInitFormatS(&format[2], L" of ");
                PhInitFormatSize(&format[3], contentLength);
                PhInitFormatS(&format[4], L" (");
                PhInitFormatF(&format[5], percent, 1);
                PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
                PhInitFormatSize(&format[7], timeBitsPerSecond);
                PhInitFormatS(&format[8], L"/s");

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                {
                    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                }

                SendMessage(Context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
            }
        }

        downloadSuccess = TRUE;
    }

CleanupExit:

    if (tempFileHandle)
        NtClose(tempFileHandle);

    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);

    if (httpConnectionHandle)
        WinHttpCloseHandle(httpConnectionHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);

    PhClearReference(&downloadHostPath);
    PhClearReference(&downloadUrlPath);
    PhClearReference(&userAgentString);
    PhClearReference(&downloadFileName);

    return downloadSuccess;
}
