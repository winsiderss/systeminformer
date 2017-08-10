/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2011-2016 dmex
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

#include "updater.h"
#include <commonutil.h>
#include <shlobj.h>

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;

PPH_UPDATER_CONTEXT CreateUpdateContext(
    _In_ BOOLEAN StartupCheck
    )
{
    PPH_UPDATER_CONTEXT context;

    context = (PPH_UPDATER_CONTEXT)PhCreateAlloc(sizeof(PH_UPDATER_CONTEXT));
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));

    PhGetPhVersionNumbers(
        &context->CurrentMajorVersion, 
        &context->CurrentMinorVersion, 
        NULL, 
        &context->CurrentRevisionVersion
        );
    context->StartupCheck = StartupCheck;

    return context;
}

VOID FreeUpdateContext(
    _In_ _Post_invalid_ PPH_UPDATER_CONTEXT Context
    )
{
    PhClearReference(&Context->Version);
    PhClearReference(&Context->RevVersion);
    PhClearReference(&Context->RelDate);
    PhClearReference(&Context->Size);
    PhClearReference(&Context->Hash);
    PhClearReference(&Context->Signature);
    PhClearReference(&Context->ReleaseNotesUrl);
    PhClearReference(&Context->SetupFilePath);
    PhClearReference(&Context->SetupFileDownloadUrl);
    PhClearReference(&Context->BuildMessage);

    PhDereferenceObject(Context);
}

VOID TaskDialogCreateIcons(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    Context->IconSmallHandle = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER));
    Context->IconLargeHandle = PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER));

    SendMessage(Context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)Context->IconSmallHandle);
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)Context->IconLargeHandle);
}

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    if (!PhIsNullOrEmptyString(Context->ReleaseNotesUrl))
    {
        DialogBoxParam(
            PluginInstance->DllBase, 
            MAKEINTRESOURCE(IDD_TEXT),
            Context->DialogHandle,
            TextDlgProc, 
            (LPARAM)Context
            );
    }
}

PPH_STRING UpdaterGetOpaqueXmlNodeText(
    _In_ mxml_node_t *xmlNode
    )
{
    if (xmlNode && xmlNode->child && xmlNode->child->type == MXML_OPAQUE && xmlNode->child->value.opaque)
    {
        return PhConvertUtf8ToUtf16(xmlNode->child->value.opaque);
    }

    return PhReferenceEmptyString();
}

BOOLEAN UpdaterInstalledUsingSetup(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ProcessHacker");
    static PH_STRINGREF key2xName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");

    HANDLE keyHandle = NULL;

    // Check uninstall entries for the 'ProcessHacker' registry key.
    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        NtClose(keyHandle);
        return TRUE;
    }

    // Check uninstall entries for the 2.x branch 'Process_Hacker2_is1' registry key.
    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &key2xName,
        0
        )))
    {
        NtClose(keyHandle);
        return TRUE;
    }

    return FALSE;
}

BOOLEAN LastUpdateCheckExpired(
    VOID
    )
{
    ULONG64 lastUpdateTimeTicks = 0;
    LARGE_INTEGER currentUpdateTimeTicks;
    PPH_STRING lastUpdateTimeString;

    PhQuerySystemTime(&currentUpdateTimeTicks);

    lastUpdateTimeString = PhGetStringSetting(SETTING_NAME_LAST_CHECK);
    PhStringToInteger64(&lastUpdateTimeString->sr, 0, &lastUpdateTimeTicks);
    PhDereferenceObject(lastUpdateTimeString);

    if (currentUpdateTimeTicks.QuadPart - lastUpdateTimeTicks >= 7 * PH_TICKS_PER_DAY)
    {
        PPH_STRING currentUpdateTimeString = PhFormatUInt64(currentUpdateTimeTicks.QuadPart, FALSE);

        PhSetStringSetting2(SETTING_NAME_LAST_CHECK, &currentUpdateTimeString->sr);

        PhDereferenceObject(currentUpdateTimeString);     
        return TRUE;
    }

    return FALSE;
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
            buildLabHeader = PhConcatStrings2(L"ProcessHacker-OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }
        else if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLab"))
        {
            buildLabHeader = PhConcatStrings2(L"ProcessHacker-OsBuild: ", buildLabString->Buffer);
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

    PhInitializeStringRef(&remaining, PhGetString(VersionString));

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

BOOLEAN QueryUpdateData(
    _Inout_ PPH_UPDATER_CONTEXT Context
    )
{
    BOOLEAN success = FALSE;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    ULONG stringBufferLength = 0;
    PSTR stringBuffer = NULL;
    PVOID jsonObject = NULL;
    mxml_node_t* xmlNode = NULL;
    PPH_STRING versionHeader = UpdateVersionString();
    PPH_STRING windowsHeader = UpdateWindowsString();
    PSTR tempValue = NULL;

    if (!(httpSessionHandle = WinHttpOpen(
        NULL,
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
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(httpRequestHandle = WinHttpOpenRequest(
        httpConnectionHandle,
        NULL,
        L"/processhacker/nightly.php?phupdater",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE
        )))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    WinHttpSetOption(
        httpRequestHandle,
        WINHTTP_OPTION_DISABLE_FEATURE, 
        &(ULONG){ WINHTTP_DISABLE_KEEP_ALIVE }, 
        sizeof(ULONG)
        );
    
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
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!ReadRequestString(httpRequestHandle, &stringBuffer, &stringBufferLength))
        goto CleanupExit;

    if (stringBuffer == NULL || stringBuffer[0] == '\0')
        goto CleanupExit;

    if (!(jsonObject = PhCreateJsonParser(stringBuffer)))
        goto CleanupExit;

    Context->Size = PhFormatSize(PhGetJsonValueAsLong64(jsonObject, "size"), 2);
    if (tempValue = PhGetJsonValueAsString(jsonObject, "version"))
    {
        Context->Version = PhConvertUtf8ToUtf16(tempValue);
        Context->RevVersion = PhConvertUtf8ToUtf16(tempValue);
    }
    if (tempValue = PhGetJsonValueAsString(jsonObject, "updated"))
        Context->RelDate = PhConvertUtf8ToUtf16(tempValue);
    if (tempValue = PhGetJsonValueAsString(jsonObject, "hash_setup"))
        Context->Hash = PhConvertUtf8ToUtf16(tempValue);
    if (tempValue = PhGetJsonValueAsString(jsonObject, "sig"))
        Context->Signature = PhConvertUtf8ToUtf16(tempValue);
    if (tempValue = PhGetJsonValueAsString(jsonObject, "forum_url"))
        Context->ReleaseNotesUrl = PhConvertUtf8ToUtf16(tempValue);
    if (tempValue = PhGetJsonValueAsString(jsonObject, "setup_url"))
        Context->SetupFileDownloadUrl = PhConvertUtf8ToUtf16(tempValue);
    if (tempValue = PhGetJsonValueAsString(jsonObject, "changelog"))
        Context->BuildMessage = PhConvertUtf8ToUtf16(tempValue);

    PhFreeJsonParser(jsonObject);

    if (PhIsNullOrEmptyString(Context->Version))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->RevVersion))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->RelDate))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->Size))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->Hash))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->ReleaseNotesUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileDownloadUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->Signature))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->BuildMessage))
        goto CleanupExit;

    success = TRUE;

CleanupExit:

    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);

    if (httpConnectionHandle)
        WinHttpCloseHandle(httpConnectionHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);

    if (xmlNode)
        mxmlDelete(xmlNode);

    if (stringBuffer)
        PhFree(stringBuffer);

    PhClearReference(&versionHeader);
    PhClearReference(&windowsHeader);

    if (!PhIsNullOrEmptyString(Context->BuildMessage))
    {
        PH_STRING_BUILDER sb;

        PhInitializeStringBuilder(&sb, 0x100);

        for (SIZE_T i = 0; i < Context->BuildMessage->Length / sizeof(WCHAR); i++)
        {
            if (Context->BuildMessage->Data[i] == '\n')
                PhAppendStringBuilder2(&sb, L"\r\n");
            else
                PhAppendCharStringBuilder(&sb, Context->BuildMessage->Data[i]);
        }

        PhMoveReference(&Context->BuildMessage, PhFinalStringBuilderString(&sb));
    }

    return success;
}

NTSTATUS UpdateCheckSilentThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context = NULL;
    ULONGLONG currentVersion = 0;
    ULONGLONG latestVersion = 0;

    context = CreateUpdateContext(TRUE);

#ifndef FORCE_UPDATE_CHECK
    if (!LastUpdateCheckExpired())
        goto CleanupExit;
#endif
    if (!QueryUpdateData(context))
        goto CleanupExit;

    currentVersion = MAKE_VERSION_ULONGLONG(
        context->CurrentMajorVersion,
        context->CurrentMinorVersion,
        context->CurrentRevisionVersion,
        0
        );

#ifdef FORCE_UPDATE_CHECK
    latestVersion = MAKE_VERSION_ULONGLONG(
        9999,
        9999,
        9999,
        0
        );
#else
    latestVersion = ParseVersionString(context->Version);
#endif

    // Compare the current version against the latest available version
    if (currentVersion < latestVersion)
    {
        // Don't spam the user the second they open PH, delay dialog creation for 3 seconds.
        //Sleep(3000);

        // Check if the user hasn't already opened the dialog.
        if (!UpdateDialogHandle)
        {
            // We have data we're going to cache and pass into the dialog
            context->HaveData = TRUE;

            // Show the dialog asynchronously on a new thread.
            ShowUpdateDialog(context);
        }
    }

CleanupExit:

    if (!context->HaveData)
        FreeUpdateContext(context);

    return STATUS_SUCCESS;
}

NTSTATUS UpdateCheckThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context = NULL;
    ULONGLONG currentVersion = 0;
    ULONGLONG latestVersion = 0;

    context = (PPH_UPDATER_CONTEXT)Parameter;
    context->ErrorCode = STATUS_SUCCESS;

    // Check if we have cached update data
    if (!context->HaveData)
    {
        context->HaveData = QueryUpdateData(context);
    }

    if (!context->HaveData)
    {
        PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);

        PhDereferenceObject(context);
        return STATUS_SUCCESS;
    }

    currentVersion = MAKE_VERSION_ULONGLONG(
        context->CurrentMajorVersion,
        context->CurrentMinorVersion,
        context->CurrentRevisionVersion,
        0
        );

#ifdef FORCE_UPDATE_CHECK
    latestVersion = MAKE_VERSION_ULONGLONG(
        9999,
        9999,
        9999,
        0
        );
#else
    latestVersion = ParseVersionString(context->Version);
#endif

    if (currentVersion == latestVersion)
    {
        // User is running the latest version
        PostMessage(context->DialogHandle, PH_UPDATEISCURRENT, 0, 0);
    }
    else if (currentVersion > latestVersion)
    {
        // User is running a newer version
        PostMessage(context->DialogHandle, PH_UPDATENEWER, 0, 0);
    }
    else
    {
        // User is running an older version
        PostMessage(context->DialogHandle, PH_UPDATEAVAILABLE, 0, 0);
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

static PPH_STRING UpdaterParseDownloadFileName(
    _In_ PPH_STRING DownloadUrlPath
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF baseNamePart;
    PPH_STRING filePath;
    PPH_STRING downloadFileName;

    if (!PhSplitStringRefAtLastChar(&DownloadUrlPath->sr, '/', &pathPart, &baseNamePart))
        return NULL;

    downloadFileName = PhCreateString2(&baseNamePart);
    filePath = PhGetCacheFileName(downloadFileName);
    PhDereferenceObject(downloadFileName);

    return filePath;
}

NTSTATUS UpdateDownloadThread(
    _In_ PVOID Parameter
    )
{
    BOOLEAN downloadSuccess = FALSE;
    BOOLEAN hashSuccess = FALSE;
    BOOLEAN signatureSuccess = FALSE;
    HANDLE tempFileHandle = NULL;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PPH_STRING userAgentString = NULL;
    PUPDATER_HASH_CONTEXT hashContext = NULL;
    URL_COMPONENTS httpParts = { sizeof(URL_COMPONENTS) };
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    // Create a user agent string.
    userAgentString = PhFormatString(
        L"PH_%lu.%lu_%lu",
        context->CurrentMajorVersion,
        context->CurrentMinorVersion,
        context->CurrentRevisionVersion
        );

    if (PhIsNullOrEmptyString(userAgentString))
        goto CleanupExit;

    // Set lengths to non-zero enabling these params to be cracked.
    httpParts.dwSchemeLength = (ULONG)-1;
    httpParts.dwHostNameLength = (ULONG)-1;
    httpParts.dwUrlPathLength = (ULONG)-1;

    if (!WinHttpCrackUrl(
        PhGetStringOrEmpty(context->SetupFileDownloadUrl),
        0,
        0,
        &httpParts
        ))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    // Create the Host string.
    downloadHostPath = PhCreateStringEx(httpParts.lpszHostName, httpParts.dwHostNameLength * sizeof(WCHAR));

    if (PhIsNullOrEmptyString(downloadHostPath))
        goto CleanupExit;

    // Create the remote path string.
    downloadUrlPath = PhCreateStringEx(httpParts.lpszUrlPath, httpParts.dwUrlPathLength * sizeof(WCHAR));

    if (PhIsNullOrEmptyString(downloadUrlPath))
        goto CleanupExit;

    // Create the local path string.
    context->SetupFilePath = UpdaterParseDownloadFileName(downloadUrlPath);

    if (PhIsNullOrEmptyString(context->SetupFilePath))
        goto CleanupExit;

    // Create output file
    if (!NT_SUCCESS(PhCreateFileWin32(
        &tempFileHandle,
        PhGetString(context->SetupFilePath),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

    // Open the HTTP session with the system proxy configuration if available
    if (!(httpSessionHandle = WinHttpOpen(
        PhGetString(userAgentString),
        WindowsVersion >= WINDOWS_8_1 ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        )))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        WinHttpSetOption(
            httpSessionHandle, 
            WINHTTP_OPTION_DECOMPRESSION, 
            &(ULONG){ WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE }, 
            sizeof(ULONG)
            );
    }

    if (!(httpConnectionHandle = WinHttpConnect(
        httpSessionHandle,
        PhGetString(downloadHostPath),
        httpParts.nScheme == INTERNET_SCHEME_HTTP ? INTERNET_DEFAULT_HTTP_PORT : INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(httpRequestHandle = WinHttpOpenRequest(
        httpConnectionHandle,
        NULL,
        PhGetString(downloadUrlPath),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | (httpParts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
        )))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    WinHttpSetOption(
        httpRequestHandle, 
        WINHTTP_OPTION_DISABLE_FEATURE, 
        &(ULONG){ WINHTTP_DISABLE_KEEP_ALIVE },
        sizeof(ULONG)
        );

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");

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
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

    if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }
    else
    {
        ULONG bytesDownloaded = 0;
        ULONG downloadedBytes = 0;
        ULONG contentLengthSize = sizeof(ULONG);
        ULONG contentLength = 0;
        PPH_STRING status;
        IO_STATUS_BLOCK isb;
        BYTE buffer[PAGE_SIZE];

        status = PhFormatString(L"Downloading update %s...", PhGetStringOrEmpty(context->Version));

        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)status->Buffer);
        PhDereferenceObject(status);

        // Start the clock.
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
            context->ErrorCode = GetLastError();
            goto CleanupExit;
        }

        // Initialize hash algorithm.
        if (!(hashContext = UpdaterInitializeHash()))
            goto CleanupExit;

        // Zero the buffer.
        memset(buffer, 0, PAGE_SIZE);

        // Download the data.
        while (WinHttpReadData(httpRequestHandle, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            // If we get zero bytes, the file was uploaded or there was an error
            if (bytesDownloaded == 0)
                break;

            // If the dialog was closed, just cleanup and exit
            if (!UpdateDialogThreadHandle)
                goto CleanupExit;

            // Update the hash of bytes we downloaded.
            UpdaterUpdateHash(hashContext, buffer, bytesDownloaded);

            // Write the downloaded bytes to disk.
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

            downloadedBytes += (DWORD)isb.Information;

            // Check the number of bytes written are the same we downloaded.
            if (bytesDownloaded != isb.Information)
                goto CleanupExit;

            // Query the current time
            PhQuerySystemTime(&timeNow);

            // Calculate the number of ticks
            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
            timeBitsPerSecond = downloadedBytes / __max(timeTicks, 1);

            // TODO: Update on timer callback.
            {
                FLOAT percent = ((FLOAT)downloadedBytes / contentLength * 100);
                PPH_STRING totalLength = PhFormatSize(contentLength, -1);
                PPH_STRING totalDownloaded = PhFormatSize(downloadedBytes, -1);
                PPH_STRING totalSpeed = PhFormatSize(timeBitsPerSecond, -1);

                PPH_STRING statusMessage = PhFormatString(
                    L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s",
                    PhGetStringOrEmpty(totalDownloaded),
                    PhGetStringOrEmpty(totalLength),
                    percent,
                    PhGetStringOrEmpty(totalSpeed)
                    );

                SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)statusMessage->Buffer);
                SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);

                PhDereferenceObject(statusMessage);
                PhDereferenceObject(totalSpeed);
                PhDereferenceObject(totalLength);
                PhDereferenceObject(totalDownloaded);
            }
        }

        if (UpdaterVerifyHash(hashContext, context->Hash))
        {
            hashSuccess = TRUE;
        }

        if (UpdaterVerifySignature(hashContext, context->Signature))
        {
            signatureSuccess = TRUE;
        }

        if (hashSuccess && signatureSuccess)
        {
            downloadSuccess = TRUE;
        }
    }

CleanupExit:

    if (hashContext)
        UpdaterDestroyHash(hashContext);

    if (tempFileHandle)
        NtClose(tempFileHandle);

    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);

    if (httpConnectionHandle)
        WinHttpCloseHandle(httpConnectionHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);

    if (downloadHostPath)
        PhDereferenceObject(downloadHostPath);

    if (downloadUrlPath)
        PhDereferenceObject(downloadUrlPath);

    if (userAgentString)
        WinHttpCloseHandle(userAgentString);

    if (UpdateDialogThreadHandle)
    {
        if (downloadSuccess && hashSuccess && signatureSuccess)
        {
            PostMessage(context->DialogHandle, PH_UPDATESUCCESS, 0, 0);
        }
        else if (downloadSuccess)
        {
            PostMessage(context->DialogHandle, PH_UPDATEFAILURE, signatureSuccess, hashSuccess);
        }
        else
        {
            PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);
        }
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
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
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwndDlg, TaskDialogSubclassProc, uIdSubclass);
        }
        break;
    case PH_UPDATEAVAILABLE:
        {
            ShowAvailableDialog(context);
        }
        break;
    case PH_UPDATEISCURRENT:
        {
            ShowLatestVersionDialog(context);
        }
        break;
    case PH_UPDATENEWER:
        {
            ShowNewerVersionDialog(context);
        }
        break;
    case PH_UPDATESUCCESS:
        {
            ShowUpdateInstallDialog(context);
        }
        break;
    case PH_UPDATEFAILURE:
        {
            if ((BOOLEAN)wParam)
                ShowUpdateFailedDialog(context, TRUE, FALSE);
            else if ((BOOLEAN)lParam)
                ShowUpdateFailedDialog(context, FALSE, TRUE);
            else
                ShowUpdateFailedDialog(context, FALSE, FALSE);
        }
        break;
    case PH_UPDATEISERRORED:
        {
            ShowUpdateFailedDialog(context, FALSE, FALSE);
        }
        break;
    //case WM_PARENTNOTIFY:
    //    {
    //        if (wParam == WM_CREATE)
    //        {
    //            // uMsg == 49251 for expand/collapse button click
    //            HWND hwndEdit = CreateWindowEx(
    //                WS_EX_CLIENTEDGE,
    //                L"EDIT",
    //                NULL,
    //                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
    //                5,
    //                5,
    //                390,
    //                85,
    //                (HWND)lParam, // parent window
    //                0,
    //                NULL,
    //                NULL
    //            );
    //
    //            PhCreateCommonFont(-11, hwndEdit);
    //
    //            // Add text to the window.
    //            SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)L"TEST");
    //        }
    //    }
    //    break;
    //case WM_NCACTIVATE:
    //    {
    //        if (IsWindowVisible(PhMainWndHandle) && !IsMinimized(PhMainWndHandle))
    //        {
    //            if (!context->FixedWindowStyles)
    //            {
    //                SetWindowLongPtr(hwndDlg, GWLP_HWNDPARENT, (LONG_PTR)PhMainWndHandle);
    //                PhSetWindowExStyle(hwndDlg, WS_EX_APPWINDOW, WS_EX_APPWINDOW);
    //                context->FixedWindowStyles = TRUE;
    //            }
    //        }
    //    }
    //    break;
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
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            UpdateDialogHandle = context->DialogHandle = hwndDlg;

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(PhMainWndHandle) && !IsMinimized(PhMainWndHandle)) ? PhMainWndHandle : NULL);

            // Create the Taskdialog icons
            TaskDialogCreateIcons(context);

            // Subclass the Taskdialog
            SetWindowSubclass(hwndDlg, TaskDialogSubclassProc, 0, (ULONG_PTR)context);

            if (context->StartupCheck)
            {
                ShowAvailableDialog(context);
            }
            else
            {
                ShowCheckForUpdatesDialog(context);
            }
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
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    if (Parameter)
        context = (PPH_UPDATER_CONTEXT)Parameter;
    else
        context = CreateUpdateContext(FALSE);

    PhInitializeAutoPool(&autoPool);

    // Start TaskDialog bootstrap
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    TaskDialogIndirect(&config, NULL, NULL, NULL);

    FreeUpdateContext(context);
    PhDeleteAutoPool(&autoPool);

    if (UpdateDialogThreadHandle)
    {
        NtClose(UpdateDialogThreadHandle);
        UpdateDialogThreadHandle = NULL;
    }

    PhResetEvent(&InitializedEvent);

    return STATUS_SUCCESS;
}

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    )
{
    if (!UpdateDialogThreadHandle)
    {
        if (!(UpdateDialogThreadHandle = PhCreateThread(0, ShowUpdateDialogThread, Context)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the updater window.", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    PostMessage(UpdateDialogHandle, WM_INITDIALOG, 0, 0);
}

VOID StartInitialCheck(
    VOID
    )
{
    PhCreateThread2(UpdateCheckSilentThread, NULL);
}