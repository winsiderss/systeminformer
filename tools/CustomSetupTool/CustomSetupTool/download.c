#include <setup.h>
#include <appsup.h>
#include <workqueue.h>
#include <winhttp.h>

#include "json-c\json.h"

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
    PPH_STRING currentVersion = 0;
    PPH_STRING versionHeader = NULL;

    if (currentVersion = SetupGetVersion())
    {
        versionHeader = PhConcatStrings2(L"PH-SETUP-BUILD: ", currentVersion->Buffer);
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
            buildLabHeader = PhConcatStrings2(L"PH-OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }
        else if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLab"))
        {
            buildLabHeader = PhConcatStrings2(L"PH-OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }

        NtClose(keyHandle);
    }

    return buildLabHeader;
}

BOOLEAN ParseVersionString(
    _Inout_ PPH_SETUP_CONTEXT Context
    )
{
    PH_STRINGREF remaining, majorPart, minorPart, revisionPart;
    ULONG64 majorInteger = 0, minorInteger = 0, revisionInteger = 0;

    PhInitializeStringRef(&remaining, PhGetStringOrEmpty(Context->Version));

    PhSplitStringRefAtChar(&remaining, '.', &majorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, '.', &minorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, '.', &revisionPart, &remaining);

    PhStringToInteger64(&majorPart, 10, &majorInteger);
    PhStringToInteger64(&minorPart, 10, &minorInteger);
    PhStringToInteger64(&revisionPart, 10, &revisionInteger);

    Context->LatestMajorVersion = (ULONG)majorInteger;
    Context->LatestMinorVersion = (ULONG)minorInteger;
    Context->LatestRevisionVersion = (ULONG)revisionInteger;
    return TRUE;
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

json_object_ptr json_get_object(
    _In_ json_object_ptr rootObj,
    _In_ const PSTR key
)
{
    json_object_ptr returnObj;

    if (json_object_object_get_ex(rootObj, key, &returnObj))
    {
        return returnObj;
    }

    return NULL;
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
        ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

        WinHttpSetOption(
            httpSessionHandle,
            WINHTTP_OPTION_DECOMPRESSION,
            &httpFlags,
            sizeof(ULONG)
            );
    }

    if (!(httpConnectionHandle = WinHttpConnect(
        httpSessionHandle,
        L"wj32.org",
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
        L"/processhacker/nightly.php?phsetup",
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
        ULONG keepAlive = WINHTTP_DISABLE_KEEP_ALIVE;
        WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &keepAlive, sizeof(ULONG));
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

    if (!(jsonObject = json_tokener_parse(stringBuffer)))
        goto CleanupExit;

    Context->Version = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "version")));
    //Context->RevVersion = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "rev")));
    Context->RelDate = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "updated")));
    Context->Size = PhFormatSize(json_object_get_int64(json_get_object(jsonObject, "size")), -1);
    Context->Hash = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "hash_setup")));
    Context->Signature = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "sig")));
    Context->ReleaseNotesUrl = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "forum_url")));
    Context->BinFileDownloadUrl = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "bin_url")));
    //Context->SetupFileDownloadUrl = PhConvertUtf8ToUtf16(json_object_get_string(json_get_object(jsonObject, "setup_url")));
    //Context->BuildMessage = PhConvertUtf8ToUtf16(GetJsonValueAsString(jsonObject, "changelog"));

    if (PhIsNullOrEmptyString(Context->Version))
        goto CleanupExit;
    if (!ParseVersionString(Context))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(Context->Signature))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->RelDate))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->Size))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->Hash))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->ReleaseNotesUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->BinFileDownloadUrl))
        goto CleanupExit;
 
    success = TRUE;

CleanupExit:

    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);

    if (httpConnectionHandle)
        WinHttpCloseHandle(httpConnectionHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);

    if (jsonObject)
        json_object_put(jsonObject);

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
    PPH_STRING setupTempPath = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING fullSetupPath = NULL;
    PPH_STRING randomGuidString = NULL;
    ULONG indexOfFileName = -1;
    GUID randomGuid;
    URL_COMPONENTS httpUrlComponents = { sizeof(URL_COMPONENTS) };
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;

    SetWindowText(Context->MainHeaderHandle, L"Initializing download request...");

    userAgentString = PhFormatString(
        L"PH_%lu.%lu_%lu",
        Context->CurrentMajorVersion,
        Context->CurrentMinorVersion,
        Context->CurrentRevisionVersion
        );

    setupTempPath = PhCreateStringEx(NULL, GetTempPath(0, NULL) * sizeof(WCHAR));

    if (GetTempPath((ULONG)setupTempPath->Length / sizeof(WCHAR), setupTempPath->Buffer) == 0)
        goto CleanupExit;

    PhGenerateGuid(&randomGuid);

    if (randomGuidString = PhFormatGuid(&randomGuid))
    {
        PhMoveReference(
            &randomGuidString, 
            PhSubstring(randomGuidString, 1, randomGuidString->Length / sizeof(WCHAR) - 2)
            );
    }

    Context->SetupFilePath = PhFormatString(
        L"%s%s\\processhacker-%lu.%lu.%lu-bin.zip",
        PhGetStringOrEmpty(setupTempPath),
        PhGetStringOrEmpty(randomGuidString),
        Context->LatestMajorVersion,
        Context->LatestMinorVersion,
        Context->LatestRevisionVersion
        );
    if (PhIsNullOrEmptyString(Context->SetupFilePath))
        goto CleanupExit;

    if (fullSetupPath = PhGetFullPath(PhGetString(Context->SetupFilePath), &indexOfFileName))
    {
        PPH_STRING directoryPath;

        if (indexOfFileName == -1)
            goto CleanupExit;

        if (directoryPath = PhSubstring(fullSetupPath, 0, indexOfFileName))
        {
            SHCreateDirectoryEx(NULL, directoryPath->Buffer, NULL);
            PhDereferenceObject(directoryPath);
        }
    }

    if (!NT_SUCCESS(PhCreateFileWin32(
        &tempFileHandle,
        PhGetString(Context->SetupFilePath),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

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

    SetWindowText(Context->MainHeaderHandle, PhFormatString(L"Downloading Process Hacker %lu.%lu.%lu...",
        Context->LatestMajorVersion,
        Context->CurrentMinorVersion,
        Context->LatestRevisionVersion
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
        ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
        WinHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DECOMPRESSION, &httpFlags, sizeof(ULONG));
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
        ULONG keepAlive = WINHTTP_DISABLE_KEEP_ALIVE;
        WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &keepAlive, sizeof(ULONG));
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
        //PPH_STRING status;
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
                PPH_STRING totalLength = PhFormatSize(contentLength, -1);
                PPH_STRING totalDownloaded = PhFormatSize(downloadedBytes, -1);
                PPH_STRING totalSpeed = PhFormatSize(timeBitsPerSecond, -1);            
                PPH_STRING statusMessage = PhFormatString(
                    L"Downloaded: %s of %s (%.0f%%)",
                    PhGetStringOrEmpty(totalDownloaded),
                    PhGetStringOrEmpty(totalLength),
                    percent
                    );
                PPH_STRING subMessage = PhFormatString(
                    L"Speed: %s/s",
                    PhGetStringOrEmpty(totalSpeed)
                    );

                SetWindowText(Context->StatusHandle, statusMessage->Buffer);
                SetWindowText(Context->SubStatusHandle, subMessage->Buffer);
                SendMessage(Context->ProgressHandle, PBM_SETPOS, (WPARAM)percent, 0);

                PhDereferenceObject(subMessage);
                PhDereferenceObject(statusMessage);
                PhDereferenceObject(totalSpeed);
                PhDereferenceObject(totalLength);
                PhDereferenceObject(totalDownloaded);
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

    PhClearReference(&randomGuidString);
    PhClearReference(&fullSetupPath);
    PhClearReference(&setupTempPath);
    PhClearReference(&downloadHostPath);
    PhClearReference(&downloadUrlPath);
    PhClearReference(&userAgentString);

    //if (UpdateDialogThreadHandle)
    //{
    //    if (downloadSuccess && hashSuccess && signatureSuccess)
    //    {
    //        PostMessage(context->DialogHandle, PH_UPDATESUCCESS, 0, 0);
    //    }
    //    else if (downloadSuccess)
    //    {
    //        PostMessage(context->DialogHandle, PH_UPDATEFAILURE, signatureSuccess, hashSuccess);
    //    }
    //    else
    //    {
    //        PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);
    //    }
    //}

    return downloadSuccess;
}
