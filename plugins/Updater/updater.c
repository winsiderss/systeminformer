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

    context = (PPH_UPDATER_CONTEXT)PhAllocate(sizeof(PH_UPDATER_CONTEXT));
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));

    context->StartupCheck = StartupCheck;

    return context;
}

VOID FreeUpdateContext(
    _In_ _Post_invalid_ PPH_UPDATER_CONTEXT Context
    )
{
    if (!Context)
        return;

    Context->HaveData = FALSE;

    Context->MinorVersion = 0;
    Context->MajorVersion = 0;
    Context->RevisionVersion = 0;
    Context->CurrentMinorVersion = 0;
    Context->CurrentMajorVersion = 0;
    Context->CurrentRevisionVersion = 0;

    PhClearReference(&Context->Version);
    PhClearReference(&Context->RevVersion);
    PhClearReference(&Context->RelDate);
    PhClearReference(&Context->Size);
    PhClearReference(&Context->Hash);
    PhClearReference(&Context->Signature);
    PhClearReference(&Context->ReleaseNotesUrl);
    PhClearReference(&Context->SetupFilePath);
    PhClearReference(&Context->SetupFileDownloadUrl);

    PhFree(Context);
}

VOID TaskDialogCreateIcons(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    // Load the Process Hacker window icon
    Context->IconLargeHandle = (HICON)LoadImage(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_SHARED
        );
    Context->IconSmallHandle = (HICON)LoadImage(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_SHARED
        );

    // Set the TaskDialog window icons
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)Context->IconSmallHandle);
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)Context->IconLargeHandle);
}

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    if (!PhIsNullOrEmptyString(Context->ReleaseNotesUrl))
    {
        // Launch the ReleaseNotes URL (if it exists) with the default browser
        PhShellExecute(Context->DialogHandle, Context->ReleaseNotesUrl->Buffer, NULL);
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
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");

    HANDLE keyHandle = NULL;

    // Check uninstall entries for the 'Process_Hacker2_is1' registry key.
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

    return FALSE;
}

BOOLEAN LastUpdateCheckExpired(
    VOID
    )
{
#ifdef FORCE_UPDATE_CHECK
    return TRUE;
#else
    ULONG64 lastUpdateTimeTicks = 0;
    LARGE_INTEGER currentUpdateTimeTicks;
    PPH_STRING lastUpdateTimeString;

    // Get the last update check time
    lastUpdateTimeString = PhGetStringSetting(SETTING_NAME_LAST_CHECK);
    PhStringToInteger64(&lastUpdateTimeString->sr, 0, &lastUpdateTimeTicks);

    // Query the current time
    PhQuerySystemTime(&currentUpdateTimeTicks);

    // Check if the last update check was more than 7 days ago
    if (currentUpdateTimeTicks.QuadPart - lastUpdateTimeTicks >= 7 * PH_TICKS_PER_DAY)
    {
        PPH_STRING currentUpdateTimeString = PhFormatUInt64(currentUpdateTimeTicks.QuadPart, FALSE);

        // Save the current time
        PhSetStringSetting2(SETTING_NAME_LAST_CHECK, &currentUpdateTimeString->sr);

        // Cleanup
        PhDereferenceObject(currentUpdateTimeString);
        PhDereferenceObject(lastUpdateTimeString);
        return TRUE;
    }

    // Cleanup
    PhDereferenceObject(lastUpdateTimeString);
    return FALSE;
#endif
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG revisionVersion;
    PPH_STRING currentVersion = NULL;
    PPH_STRING versionHeader = NULL;

    PhGetPhVersionNumbers(
        &majorVersion,
        &minorVersion,
        NULL,
        &revisionVersion
        );

    currentVersion = PhFormatString(
        L"%lu.%lu.%lu",
        majorVersion,
        minorVersion,
        revisionVersion
        );

    if (currentVersion)
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

    HANDLE keyHandle = NULL;
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

BOOLEAN ParseVersionString(
    _Inout_ PPH_UPDATER_CONTEXT Context
    )
{
    PH_STRINGREF sr, majorPart, minorPart, revisionPart;
    ULONG64 majorInteger = 0, minorInteger = 0, revisionInteger = 0;

    PhInitializeStringRef(&sr, Context->Version->Buffer);
    PhInitializeStringRef(&revisionPart, Context->RevVersion->Buffer);

    if (PhSplitStringRefAtChar(&sr, '.', &majorPart, &minorPart))
    {
        PhStringToInteger64(&majorPart, 10, &majorInteger);
        PhStringToInteger64(&minorPart, 10, &minorInteger);
        PhStringToInteger64(&revisionPart, 10, &revisionInteger);

        Context->MajorVersion = (ULONG)majorInteger;
        Context->MinorVersion = (ULONG)minorInteger;
        Context->RevisionVersion = (ULONG)revisionInteger;

        return TRUE;
    }

    return FALSE;
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

BOOLEAN QueryUpdateData(
    _Inout_ PPH_UPDATER_CONTEXT Context
    )
{
    BOOLEAN isSuccess = FALSE;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
    mxml_node_t* xmlNode = NULL;
    ULONG xmlStringBufferLength = 0;
    PSTR xmlStringBuffer = NULL;
    PPH_STRING versionHeader = UpdateVersionString();
    PPH_STRING windowsHeader = UpdateWindowsString();

    // Get the current Process Hacker version
    PhGetPhVersionNumbers(
        &Context->CurrentMajorVersion,
        &Context->CurrentMinorVersion,
        NULL,
        &Context->CurrentRevisionVersion
        );

    __try
    {
        // Query the current system proxy
        WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        // Open the HTTP session with the system proxy configuration if available
        if (!(httpSessionHandle = WinHttpOpen(
            NULL,
            proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            proxyConfig.lpszProxy,
            proxyConfig.lpszProxyBypass,
            0
            )))
        {
            __leave;
        }

        if (WindowsVersion >= WINDOWS_8_1)
        {
            // Enable GZIP and DEFLATE support on Windows 8.1 and above using undocumented flags.
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
            __leave;
        }

        if (!(httpRequestHandle = WinHttpOpenRequest(
            httpConnectionHandle,
            NULL,
            L"/processhacker/update.php",
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE
            )))
        {
            __leave;
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
            __leave;
        }

        if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
            __leave;

        // Read the resulting xml into our buffer.
        if (!ReadRequestString(httpRequestHandle, &xmlStringBuffer, &xmlStringBufferLength))
            __leave;

        // Check the buffer for valid data.
        if (xmlStringBuffer == NULL || xmlStringBuffer[0] == '\0')
            __leave;

        // Load our XML
        xmlNode = mxmlLoadString(NULL, xmlStringBuffer, MXML_OPAQUE_CALLBACK);
        if (xmlNode == NULL || xmlNode->type != MXML_ELEMENT)
            __leave;

        // Find the version node
        Context->Version = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "ver", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->Version))
            __leave;

        // Find the revision node
        Context->RevVersion = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "rev", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->RevVersion))
            __leave;

        // Find the release date node
        Context->RelDate = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "reldate", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->RelDate))
            __leave;

        // Find the size node
        Context->Size = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "size", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->Size))
            __leave;

        // Find the Hash node
        Context->Hash = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "sha2", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->Hash))
            __leave;

        // Find the signature node
        Context->Signature = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "sig", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->Signature))
            __leave;

        // Find the release notes URL
        Context->ReleaseNotesUrl = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "relnotes", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->ReleaseNotesUrl))
            __leave;

        // Find the installer download URL
        Context->SetupFileDownloadUrl = UpdaterGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "setupurl", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(Context->SetupFileDownloadUrl))
            __leave;

        if (!ParseVersionString(Context))
            __leave;

        isSuccess = TRUE;
    }
    __finally
    {
        if (httpRequestHandle)
            WinHttpCloseHandle(httpRequestHandle);

        if (httpConnectionHandle)
            WinHttpCloseHandle(httpConnectionHandle);

        if (httpSessionHandle)
            WinHttpCloseHandle(httpSessionHandle);

        if (xmlNode)
            mxmlDelete(xmlNode);

        if (xmlStringBuffer)
            PhFree(xmlStringBuffer);

        PhClearReference(&versionHeader);
        PhClearReference(&windowsHeader);
    }

    return isSuccess;
}

NTSTATUS UpdateCheckSilentThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context = NULL;
    ULONGLONG currentVersion = 0;
    ULONGLONG latestVersion = 0;

    context = CreateUpdateContext(TRUE);

    __try
    {
#ifdef FORCE_NO_INTERNET
        __leave;
#endif
        if (!LastUpdateCheckExpired())
        {
            __leave;
        }

        if (!QueryUpdateData(context))
        {
            __leave;
        }

        currentVersion = MAKE_VERSION_ULONGLONG(
            context->CurrentMajorVersion,
            context->CurrentMinorVersion,
            context->CurrentRevisionVersion,
            0
            );

#ifdef FORCE_UPDATE_CHECK
#ifdef FORCE_LATEST_VERSION
        latestVersion = MAKE_VERSION_ULONGLONG(
            context->CurrentMajorVersion,
            context->CurrentMinorVersion,
            context->CurrentRevisionVersion,
            0
            );
#else
        latestVersion = MAKE_VERSION_ULONGLONG(
            9999,
            9999,
            9999,
            0
            );
#endif
#else
        latestVersion = MAKE_VERSION_ULONGLONG(
            context->MajorVersion,
            context->MinorVersion,
            context->RevisionVersion,
            0
            );
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
    }
    __finally
    {
        // Check the dialog doesn't own the window context...
        if (!context->HaveData)
        {
            FreeUpdateContext(context);
        }
    }

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

#ifdef FORCE_NO_INTERNET
    PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);
    return STATUS_SUCCESS;
#endif

    // Check if we have cached update data
    if (!context->HaveData)
    {
        context->HaveData = QueryUpdateData(context);
    }

    if (!context->HaveData) // sanity check
    {
        PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);
        return STATUS_SUCCESS;
    }

    currentVersion = MAKE_VERSION_ULONGLONG(
        context->CurrentMajorVersion,
        context->CurrentMinorVersion,
        context->CurrentRevisionVersion,
        0
        );

#ifdef FORCE_UPDATE_CHECK
#ifdef FORCE_LATEST_VERSION
    latestVersion = MAKE_VERSION_ULONGLONG(
        context->CurrentMajorVersion,
        context->CurrentMinorVersion,
        context->CurrentRevisionVersion,
        0
        );
#else
    latestVersion = MAKE_VERSION_ULONGLONG(
        9999,
        9999,
        9999,
        0
        );
#endif
#else
    latestVersion = MAKE_VERSION_ULONGLONG(
        context->MajorVersion,
        context->MinorVersion,
        context->RevisionVersion,
        0
        );
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

    return STATUS_SUCCESS;
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
    PPH_STRING setupTempPath = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING fullSetupPath = NULL;
    PPH_STRING randomGuidString = NULL;
    PUPDATER_HASH_CONTEXT hashContext = NULL;
    ULONG indexOfFileName = -1;
    GUID randomGuid;
    URL_COMPONENTS httpUrlComponents = { sizeof(URL_COMPONENTS) };
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    __try
    {
        // Create a user agent string.
        userAgentString = PhFormatString(
            L"PH_%lu.%lu_%lu",
            context->CurrentMajorVersion,
            context->CurrentMinorVersion,
            context->CurrentRevisionVersion
            );
        if (PhIsNullOrEmptyString(userAgentString))
            __leave;

        // Allocate the GetTempPath buffer
        setupTempPath = PhCreateStringEx(NULL, GetTempPath(0, NULL) * sizeof(WCHAR));
        if (PhIsNullOrEmptyString(setupTempPath))
            __leave;

        // Get the temp path
        if (GetTempPath((ULONG)setupTempPath->Length / sizeof(WCHAR), setupTempPath->Buffer) == 0)
            __leave;
        if (PhIsNullOrEmptyString(setupTempPath))
            __leave;

        // Generate random guid for our directory path.
        PhGenerateGuid(&randomGuid);

        if (randomGuidString = PhFormatGuid(&randomGuid))
        {
            PPH_STRING guidSubString;

            // Strip the left and right curly brackets.
            guidSubString = PhSubstring(randomGuidString, 1, randomGuidString->Length / sizeof(WCHAR) - 2);

            PhSwapReference(&randomGuidString, guidSubString);
        }

        // Append the tempath to our string: %TEMP%RandomString\\processhacker-%lu.%lu-setup.exe
        // Example: C:\\Users\\dmex\\AppData\\Temp\\ABCD\\processhacker-2.90-setup.exe
        context->SetupFilePath = PhFormatString(
            L"%s%s\\processhacker-%lu.%lu-setup.exe",
            setupTempPath->Buffer,
            randomGuidString->Buffer,
            context->MajorVersion,
            context->MinorVersion
            );
        if (PhIsNullOrEmptyString(context->SetupFilePath))
            __leave;

        // Create the directory if it does not exist.
        if (fullSetupPath = PhGetFullPath(context->SetupFilePath->Buffer, &indexOfFileName))
        {
            PPH_STRING directoryPath;

            if (indexOfFileName == -1)
                __leave;

            if (directoryPath = PhSubstring(fullSetupPath, 0, indexOfFileName))
            {
                SHCreateDirectoryEx(NULL, directoryPath->Buffer, NULL);
                PhDereferenceObject(directoryPath);
            }
        }

        // Create output file
        if (!NT_SUCCESS(PhCreateFileWin32(
            &tempFileHandle,
            context->SetupFilePath->Buffer,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            __leave;
        }

        // Set lengths to non-zero enabling these params to be cracked.
        httpUrlComponents.dwSchemeLength = (ULONG)-1;
        httpUrlComponents.dwHostNameLength = (ULONG)-1;
        httpUrlComponents.dwUrlPathLength = (ULONG)-1;

        if (!WinHttpCrackUrl(
            context->SetupFileDownloadUrl->Buffer,
            (ULONG)context->SetupFileDownloadUrl->Length,
            0,
            &httpUrlComponents
            ))
        {
            __leave;
        }

        // Create the Host string.
        downloadHostPath = PhCreateStringEx(
            httpUrlComponents.lpszHostName,
            httpUrlComponents.dwHostNameLength * sizeof(WCHAR)
            );
        if (PhIsNullOrEmptyString(downloadHostPath))
            __leave;

        // Create the Path string.
        downloadUrlPath = PhCreateStringEx(
            httpUrlComponents.lpszUrlPath,
            httpUrlComponents.dwUrlPathLength * sizeof(WCHAR)
            );
        if (PhIsNullOrEmptyString(downloadUrlPath))
            __leave;

        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

        // Query the current system proxy
        WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        // Open the HTTP session with the system proxy configuration if available
        if (!(httpSessionHandle = WinHttpOpen(
            userAgentString->Buffer,
            proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            proxyConfig.lpszProxy,
            proxyConfig.lpszProxyBypass,
            0
            )))
        {
            __leave;
        }

        if (WindowsVersion >= WINDOWS_8_1)
        {
            // Enable GZIP and DEFLATE support on Windows 8.1 and above using undocumented flags.
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
            downloadHostPath->Buffer,
            httpUrlComponents.nScheme == INTERNET_SCHEME_HTTP ? INTERNET_DEFAULT_HTTP_PORT : INTERNET_DEFAULT_HTTPS_PORT,
            0
            )))
        {
            __leave;
        }

        if (!(httpRequestHandle = WinHttpOpenRequest(
            httpConnectionHandle,
            NULL,
            downloadUrlPath->Buffer,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_REFRESH | (httpUrlComponents.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
            )))
        {
            __leave;
        }

        if (WindowsVersion >= WINDOWS_7)
        {
            ULONG keepAlive = WINHTTP_DISABLE_KEEP_ALIVE;
            WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &keepAlive, sizeof(ULONG));
        }

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
            __leave;
        }

        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

        if (WinHttpReceiveResponse(httpRequestHandle, NULL))
        {
            ULONG bytesDownloaded = 0;
            ULONG downloadedBytes = 0;
            ULONG contentLengthSize = sizeof(ULONG);
            ULONG contentLength = 0;
            PPH_STRING status;
            IO_STATUS_BLOCK isb;
            BYTE buffer[PAGE_SIZE];

            status = PhFormatString(L"Downloading update %lu.%lu.%lu...",
                context->MajorVersion,
                context->MinorVersion,
                context->RevisionVersion
                );

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
                __leave;
            }

            // Initialize hash algorithm.
            if (!UpdaterInitializeHash(&hashContext))
                __leave;

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
                    __leave;

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
                    __leave;
                }

                downloadedBytes += (DWORD)isb.Information;

                // Check the number of bytes written are the same we downloaded.
                if (bytesDownloaded != isb.Information)
                    __leave;

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
                        totalDownloaded->Buffer,
                        totalLength->Buffer,
                        percent,
                        totalSpeed->Buffer
                        );

                    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)statusMessage->Buffer);
                    // Update the progress bar position
                    SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);

                    PhDereferenceObject(statusMessage);
                    PhDereferenceObject(totalSpeed);
                    PhDereferenceObject(totalLength);
                    PhDereferenceObject(totalDownloaded);
                }
            }

            downloadSuccess = TRUE;

            if (UpdaterVerifyHash(hashContext, context->Hash))
            {
                hashSuccess = TRUE;
            }

            if (UpdaterVerifySignature(hashContext, context->Signature))
            {
                signatureSuccess = TRUE;
            }
        }
    }
    __finally
    {
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

        PhClearReference(&randomGuidString);
        PhClearReference(&fullSetupPath);
        PhClearReference(&setupTempPath);
        PhClearReference(&downloadHostPath);
        PhClearReference(&downloadUrlPath);
        PhClearReference(&userAgentString);
    }

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
    case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwndDlg, TaskDialogSubclassProc, uIdSubclass);
        }
        break;
    case WM_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
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
    //            CommonCreateFont(-11, hwndEdit);
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

VOID ShowInitialDialog(
    _In_ PVOID Context
    )
{
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogBootstrapCallback;

    // Start TaskDialog bootstrap
    TaskDialogIndirect(&config, NULL, NULL, NULL);
}

NTSTATUS ShowUpdateDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;

    if (Parameter)
        context = (PPH_UPDATER_CONTEXT)Parameter;
    else
        context = CreateUpdateContext(FALSE);

    PhInitializeAutoPool(&autoPool);

    // Start the TaskDialog bootstrap.
    ShowInitialDialog(context);

    FreeUpdateContext(context);

    PhDeleteAutoPool(&autoPool);

    PhResetEvent(&InitializedEvent);

    if (UpdateDialogHandle)
    {
        UpdateDialogHandle = NULL;
    }

    if (UpdateDialogThreadHandle)
    {
        NtClose(UpdateDialogThreadHandle);
        UpdateDialogThreadHandle = NULL;
    }

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

    PostMessage(UpdateDialogHandle, WM_SHOWDIALOG, 0, 0);
}

VOID StartInitialCheck(
    VOID
    )
{
#ifndef DISABLE_STARTUP_CHECK
    HANDLE silentCheckThread = NULL;

    if (silentCheckThread = PhCreateThread(0, UpdateCheckSilentThread, NULL))
        NtClose(silentCheckThread);
#endif
}